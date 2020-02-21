# rfid-storeroom
Arduino RFID logger for tracking objects leaving storeroom.

Scanner allows for logging each rfid tag on SD card in csv format.<br>
Each day is separate file containing date, time, tag UID and tag data field.

Time between scans is hard coded to 400ms to prevent double scanning.<br>
Time between scanning the same tag is hard coded to 20s to prevent double scanning the same item if scanning few items in a row.

Components:
- Arduino Uno
- DS1307 RealTimeClock
- MFRC522 NFC Tag Reader
- SD Card
- Buzzer
- 2 x LED (Red and Green)
