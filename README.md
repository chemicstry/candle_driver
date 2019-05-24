# candle_driver

Python wrapper for the candle (gs_usb) windows driver which is published [here](https://github.com/HubertD/cangaroo/tree/master/src/driver/CandleApiDriver/api).

## Example usage

```python
import candle_driver

# lists all available candle devices
devices = candle_driver.list_devices()

# use first availabel device
device = devices[0]

print('Device path: %s' % device.path())
print('Device name: %s' % device.name())
print('Device timestamp: %d' % device.timestamp()) # in usec
print('Device channels: %s', device.channel_count())

# open device (blocks other processes from using it)
device.open()

# open first channel
ch = device.channel(0)

ch.set_bitrate(1000000)
# or
ch.set_timings(prop_seg=1, phase_seg1=12, phase_seg2=2, sjw=1, brp=3)

# start receiving data
ch.start()

# normal frame
ch.write(10, b'abcdefgh')
# extended frame
ch.write(10235 | candle_driver.CANDLE_ID_EXTENDED, b'abcdefgh')

# wait 1000ms for data
frame_type, can_id, can_data, extended, ts = ch.read(1000)

# close everything
ch.stop()
device.close()
```

## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details.

Windows C driver is licensed under the GPLv3 License - see the [LICENSE](candle_api/LICENSE) file for details.

## Acknowledgments

* [Hubert Denkmair](https://github.com/HubertD) for the Windows C driver.
