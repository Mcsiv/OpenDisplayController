# OpenDisplayController
Open source display controller firmware and programmer tool

## Firmware 
Only draft and garbage, nothing uploaded yet

Current task list:
- [ ] Support for RTD2660
- [X] MCU initialization
- [X] Flash initialization
- [ ] VGA support
- [ ] HDMI support
- [ ] Composite support
- [ ] OSD support
- [ ] Keyboard support
- [ ] IrDA support

## Programmer tool
Direct programming support through linux i2c driver. You can use a dedicated i2c hardware for programming or if your video card has an i2c support, you can do it through a standard VGA or HDMI cable.

**Highly experimental / Use for your own risk**

Current task list:

- [x] Support for RTD2660/RTD2662 display controllers and variants
- [x] Support for ST / Winbond / Atmel / Microchip flash chips
- [x] Firmware download 
- [x] Firmware upload
- [x] Hardware based CRC check
- [x] cmake based build system
- [x] Eliminate magic numbers / using enums everywhere
- [x] Fast upload if the flash has "chip erase" capability (Skip empty regions)
- [x] Tonnnns of comment
- [ ] Windows support through nvidia sdk
