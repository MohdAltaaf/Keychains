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
<img width="587" height="323" alt="Screenshot 2026-10-27 171224" src="https://github.com/user-attachments/assets/84aa27c3-f9c3-4c30-be91-5cd6163eed81" />

 
### 3. modelling the case
 
designed the enclosure in CAD so everything fits: board, screen, battery and switch.
<img width="1917" height="870" alt="Screenshot 2026-10-27 165213" src="https://github.com/user-attachments/assets/00bbf0b8-c133-4dea-9962-a8754a62fb91" />

 
### 4. wiring diagram
 
quick one. it's only an I2C screen, a battery and a switch, so this was the easy part.
<img width="957" height="813" alt="Screenshot 2026-10-27 173634" src="https://github.com/user-attachments/assets/dcf16d2f-acee-4443-8046-55dde81f34b1" />

 
### 5. the code
 
wrote the code for the face animations and the pairing. i hadn't worked with the BLE library before, so i got help from AI to figure it out and get the proximity part working. i still had to understand what it was doing so i could tweak it (scan timing, signal thresholds and all that).
<img width="1917" height="993" alt="image" src="https://github.com/user-attachments/assets/1d0e20ea-8d85-4bb5-b83c-40d7a657342e" />

 
### wrap up
 
everything done in about 6 hours today: idea, parts, case, wiring, code.
