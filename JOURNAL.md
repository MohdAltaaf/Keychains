## 27 October 2026: 6 hours
 
**Project:** Pair Keychains
**Total time:** about 6 hours, start to finish, all in one day
 
today i went from "wait, what if..." to a working pair of keychains. here's how the day went.
 
### 1. the idea
 
had the concept for the project. two keychains with a tiny animated face each. when they're close together they get happy and show hearts, and when they're apart they get sad. like a little long-distance thing you carry around.
 
### 2. figuring out the parts
 
worked out what i'd need and what would actually fit in a keychain:
 
- XIAO ESP32 (tiny, has Bluetooth, and charges a LiPo on its own)
- 1.3" OLED for the face
- small LiPo battery
- slide switch to turn it on and off
- 3D-printed case
decided on Bluetooth (BLE) for the pairing so no wifi or phone is needed.
 
### 3. modelling the case
 
designed the enclosure in CAD so everything fits: board, screen, battery and switch.
 
### 4. wiring diagram
 
quick one. it's only an I2C screen, a battery and a switch, so this was the easy part.
 
### 5. the code
 
wrote the code for the face animations and the pairing. i hadn't worked with the BLE library before, so i got help from AI to figure it out and get the proximity part working. i still had to understand what it was doing so i could tweak it (scan timing, signal thresholds and all that).
 
### wrap up
 
everything done in about 6 hours today: idea, parts, case, wiring, code.
