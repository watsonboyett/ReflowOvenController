
## Revision History

### v1A

* Initial design

### v1B

* Renamed to ThermoBot (formerly Tempatroller)
* Replaced 2.5mm terminal connectors with 5mm and moved them to allow for easier access
* Replaced AC-DC power supply with smaller footprint
* Replace TRIAC with SSR (for higher current rating)
* Added power and heartbeat status LEDs
* Added terminal header for powering a heatsink fan
* Improved layout


### Not-Yet-Implemented Changes

* provide safety mechanism for using external SSR (ensure that internal SSR isn't active)
** perhaps a jumper for setting internal/external SSR
* provide breakout connections for unused pins (for switches, LCD panel, or other sensors)
* Add "Risk of Shock" warnings to MAINS partition (both sides)

