
## Revision History

### v1A

* Initial design

### v1B

* Renamed to ThermoBot (formerly Tempatroller)
* Replaced 2.5mm terminal connectors with 5mm and moved them to allow for easier access
* Added power and heartbeat status LEDs
* Improved layout


### Not-Yet-Implemented Changes

* provide safety mechanism for using external SSR (ensure that internal SSR isn't active)
** perhaps a jumper for setting internal/external SSR
* provide breakout connections for unused pins (for switches, LCD panel, or other sensors)
* Add "Risk of Shock" warnings to MAINS partition (both sides)

* Replace AC-DC power supply with 1470-1110-ND (smaller footprint)
* Replace SSR with CC1607-ND (random fire) or CC1341-ND (zero-cross fire)

