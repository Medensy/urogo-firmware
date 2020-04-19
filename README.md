# UROGO
### Device states
- Normal mode - After device was powered, it enters normal mode to test all drivers, sync time with server and get back to sleep.
- Maintenance mode - After device was powered with maintenance config pin high, it enters maintenance mode. Command line interface can be use to configure device parameters including setting serial number of the device and calibrating loadcell.
- User interupt mode - After user push the button, the device enters this mode to start collecting data from loadcell and upload data to server.
- Timer interupt mode - After a period of time, the device wake up to keep real time clock in sync with time server.

### Maintenance mode command line interface
- INIT - Initialize all drivers. ** Please run init before use load cell related command (eg. CAL, WEIGHT) **
- DEINIT - Deinitialize all drivers. ** Please run deinit before exit, or reboot to prevent modules from draining current. **
- EXIT - Exit from maintenance mode, get back to sleep.
- PRINT - Print device stored parameters
- GET_SN - Get serial number of the device
- SET_SN [serial number] - Set serial number of the device
- CAL 0 - Calibrate load cell with weight of zero
- CAL [weight] - Calibrate load cell with known weight
- WEIGHT - Get weight from load cell
- SAVE - Save serial number, calibration parameters to flash memory. If not save the device uses previous stored parameters.
