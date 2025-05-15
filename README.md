| Supported Targets | ESP32-S2 | ESP32-S3 |
| ----------------- | -------- | -------- |

# ESP32-S3 USB Bluetooth LE Dongle

Turns your ESP32-S2 or S3 into a BLE dongle, which you can connect over USB to your computer or laptop.

This is done by registering the ESP32 as a USB-UART device on the computer and running Bluedroid with a VHCI host, and then copying the bytes between the two "manually" in the program using callbacks.

The code is mostly the Bluetooth VHCI and USB UART examples mashed together. Should work out-of-the-box.

Inspired by [this implementation for the normal ESP32](https://github.com/dakhnod/ESP32-Bluetooth-USB-dongle)

## Building

Simply run:
```
idf.py build flash
```
and the project should install itself to your ESP32-S3 as usual.

Note that I have enabled most `menuconfig` options, including support for BT 4.2, with the goal of having as wide of a support out-of-the-box as possible. Feel free to tweak those (and all buffer sizes) for more optimal results.

## Usage

This project assumes a Linux host with a running bluetooth service.

Connect the ESP32's USB to your computer. (For the DevKit boards, see below. For all other use cases, you should look up the correct wiring in the docs.)

Check the registered serial device name with `ls /dev/tty*`, for example `/dev/ttyACM0` or `/dev/ttyUSB0`.

Run `sudo hciattach /dev/ttyACM0 any 921600 noflow` to register the board with the bluetooth service. You should see `Device setup complete` or a similar message. Exchange `/dev/ttyACM0` with the device name from the previous step.

Any baud rate should work, considering that no actual physical communication is happening. However, keep in mind that setting it too low will create a bottleneck and might lead to unreliable operation. In my testing `115200` also worked just fine.

Check the Bluetooth device name with `hciconfig`. Look for output like this:
```
hciX:	Type: Primary  Bus: UART
	BD Address: XX:XX:XX:XX:XX:XX  ACL MTU: 251:12  SCO MTU: 0:0
	UP RUNNING 
	RX bytes:305 acl:0 sco:0 events:26 errors:0
	TX bytes:189 acl:0 sco:0 commands:26 errors:0

```

Now the ESP32-S3 should be accessible and usable as any other Bluetooth adapter from your OS.

If you want to disable it, run `killall hciattach`.

Note that resetting the ESP, or unplugging/plugging it back will stop the `hciattach` process, and you will need to re-register it again. Also, the device name might change, for example it may become `/dev/ttyACM1`.

For command-line usage, `hcitool` is deprecated will not work, as it does not support BLE. Use `bluetoothctl` instead.

## Remarks

- Only tested on Linux as of now. I am not sure if Windows is possible to support at all, as that would require a HCI-over-UART driver.
- Only supports the S2 and S3 (P4 should work as well, but not tested and might need tweaks). For the normal ESP32, see [this project.](https://github.com/dakhnod/ESP32-Bluetooth-USB-dongle)
- Might stutter for audio applications.
- On the `ESP32-S3-DevKitC-1` and `ESP32-S3-DevKitM-1`, the device-facing end of the cable should be plugged into the port labelled `USB`, **not** the `COM` port. The `COM` port is still used to send debug / info messages. You can plug in and use both at the same time, just be careful to initialize the correct one with `hciattach`.


