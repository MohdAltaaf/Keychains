# Pair Keychains

so i made a pair of little keychains with a tiny face on them. each one has an OLED screen with eyes and a mouth, and the face changes depending on whether the other keychain is close or not.

when they're together they get all happy and show hearts. when they're apart they get sad and sleepy. that's basically the whole idea.

## what it does

- shows a cute animated face (blinking, looking around, smooth transitions between emotions)
- finds its partner over Bluetooth (BLE), no wifi, no phone, no router
- checks how strong the partner's signal is to guess how close it is
- **together** → hearts, happy, surprised when you first meet
- **nearby** → mostly neutral and happy
- **apart** → sad, sleepy, neutral
- the mood stays the same until the next scan says otherwise

both keychains run the **exact same code**. no "left" and "right" version, just flash the same thing on both.

## what you need (per keychain)

- Seeed XIAO ESP32 (C3, S3 or C6 all work, i used the one with the built-in LiPo charger)
- 128x64 SH1106 I2C OLED
- 3.7V LiPo battery (with a protection circuit)
- a small slide switch
- some wire and a way to hold it all together

## wiring

| OLED | XIAO |
|------|------|
| VCC  | 3V3  |
| GND  | GND  |
| SDA  | D4   |
| SCL  | D5   |

battery goes on the BAT+ / BAT- pads on the underside of the board. put the switch on the **+ wire** so it cuts power when it's off. **double check the polarity** before connecting, reversing it can kill the board or the battery.

the board charges the battery over USB-C, so you don't need a separate charger. heads up: when USB is plugged in the board turns on no matter what the switch says.

## setup

1. install the Arduino IDE
2. add the ESP32 board package (Boards Manager → "esp32" by Espressif)
3. install the **U8g2** library (Library Manager)
4. open the sketch, select your XIAO ESP32 board
5. **change `PAIR_UUID`** to your own random one (grab one from uuidgenerator.net). both keychains need the same UUID, and it's what stops them from reacting to someone else's keychain
6. flash both keychains with the same code

BLE comes with the ESP32 core, nothing extra to install for that.

## how the pairing works

each keychain does two things at once:

1. **advertises** its UUID constantly so the other one can find it
2. **scans** for the partner's UUID for 1 second, once a minute

after each scan it looks at the strongest signal (RSSI) it saw:

- didn't see the partner → apart
- saw it and the signal is strong → together
- saw it but the signal is weak → nearby

there's some hysteresis built in (`TOGETHER_ENTER` and `TOGETHER_EXIT`) so it doesn't flicker between together and nearby when you're right on the edge.

## settings you can mess with

| setting | what it does | default |
|---------|--------------|---------|
| `SCAN_SECONDS` | how long each scan lasts | 1 |
| `SCAN_REST_MS` | wait between scans | 59000 |
| `TOGETHER_ENTER` | signal needed to count as together | -60 |
| `TOGETHER_EXIT` | signal below this = no longer together | -70 |
| `EASING` | how fast the face animates | 0.25 |
| `ROUNDNESS` | how round the eyes/mouth are | 0.45 |

## calibrating the distance

the -60 / -70 numbers are just guesses. your body, pockets and bags mess with the signal a lot, so do this:

1. open the Serial Monitor at 115200
2. you'll see lines like `Scan done: found=1 rssi=-58`
3. carry both keychains the way you actually would (pocket, bag, whatever) and note the numbers when they're close vs far
4. adjust `TOGETHER_ENTER` and `TOGETHER_EXIT` to match

roughly, -45 to -60 is arm's reach and -70 to -85 is across a room, but yours will be different.

## battery life

this is a rough estimate, not something i measured properly, so take it with salt.

with a **150 mAh** battery the stock code should last around **2 hours**. the CPU never sleeps and the screen is always on, so it eats power.

easy things that help:

- `setCpuFrequencyMhz(80);` at the start of `setup()`
- `u8g2.setContrast(40);` after `u8g2.begin()` to dim the screen
- `delay(5);` at the end of `loop()` so the CPU can idle
- a screen timeout with `u8g2.setPowerSave(1)` (needs some way to wake it, like a button)

with the first three you'd get maybe 3-4 hours. a bigger battery (400-500 mAh) helps a lot if you can fit it.

deep sleep isn't an option because the keychain has to keep advertising for its partner to find it.

## known stuff / limitations

- with one scan per minute it can take up to a minute to notice you got close (or went away). the two keychains aren't synced, so each does its own thing. want it faster? lower `SCAN_REST_MS`, but it costs battery
- sometimes a 1 second scan misses the partner and it flips to "apart" for a minute. if that happens, bump `SCAN_SECONDS` to 2
- if it doesn't compile on a newer ESP32 core because of the BLE `onResult` callback, you might need to switch to NimBLE-Arduino
- if the screen stays blank, check the wiring first and then that your OLED is at I2C address 0x3C

## ideas for later

- button or motion sensor to wake the screen
- a lonely face if they've been apart for a long time
- a big heart burst when they meet again
- better battery life with light sleep

## files

- `keychain.ino` the whole sketch

---

made for fun, feel free to copy it and make your own pair.
