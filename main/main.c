#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "tinyusb.h"
#include "tusb_cdc_acm.h"
#include <string.h>

static const char *TAG = "VHCI_USB";

// HCI buffer for TX from host to controller
#define HCI_RX_BUF_SIZE 256
static uint8_t hci_rx_buf[HCI_RX_BUF_SIZE];
static size_t hci_rx_len = 0;

// Callback when ESP32 has HCI data for host
static void host_recv_cb(uint8_t *data, uint16_t len) {
  tinyusb_cdcacm_write_queue(0, data, len);
  tinyusb_cdcacm_write_flush(0, 0);
  // ESP_LOGI(TAG, "Sent data to host.");
}

// Callback when host can send more data
static void host_send_available_cb(void) {
  // You could use this to retry sending if previous call failed
  // ESP_LOGI(TAG, "Host can send to HCI controller now.");
}

// USB CDC RX callback (host → ESP32)
void tinyusb_cdc_rx_callback(int itf, cdcacm_event_t *event) {
  size_t rx_size = 0;
  esp_err_t ret =
      tinyusb_cdcacm_read(itf, hci_rx_buf, HCI_RX_BUF_SIZE, &rx_size);

  if (ret == ESP_OK && rx_size > 0) {
    if (esp_vhci_host_check_send_available()) {
      esp_vhci_host_send_packet(hci_rx_buf, rx_size);
    } else {
      ESP_LOGI(TAG, "HCI controller not ready for TX");
    }
  }
}

// Not used but required for TinyUSB
void tinyusb_cdc_line_state_changed_callback(int itf, cdcacm_event_t *event) {
  // Handle DTR/RTS if needed
  ESP_LOGI(TAG, "CDC Line state changed.");
}

void app_main(void) {
  // TinyUSB init
  ESP_LOGI(TAG, "Starting main.");
  const tinyusb_config_t tusb_cfg = {
      .device_descriptor = NULL,
      .string_descriptor = NULL,
      .external_phy = false,
      .configuration_descriptor = NULL,
  };
  ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));
  tinyusb_config_cdcacm_t acm_cfg = {
      .usb_dev = TINYUSB_USBDEV_0,
      .cdc_port = TINYUSB_CDC_ACM_0,
      .rx_unread_buf_sz = 64,
      .callback_rx =
          &tinyusb_cdc_rx_callback, // the first way to register a callback
      .callback_rx_wanted_char = NULL,
      .callback_line_state_changed = NULL,
      .callback_line_coding_changed = NULL};

  ESP_ERROR_CHECK(tusb_cdc_acm_init(&acm_cfg));
  ESP_LOGI(TAG, "USB Init ready.");

  esp_err_t ret;
  /* Initialize NVS — it is used to store PHY calibration data */
  ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  // VHCI init
  static esp_vhci_host_callback_t vhci_callbacks = {
      .notify_host_recv = host_recv_cb,
      .notify_host_send_available = host_send_available_cb,
  };

  // ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BLE));

  esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
  ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BLE));
  esp_vhci_host_register_callback(&vhci_callbacks);

  ESP_LOGI(TAG, "VHCI over USB ready.");
}
