
# Tinkerbell
An arduino project that will ring a doorbell when someone appears on the door. It will also send a notification on your phone in the Blynk app where you can choose to take a picture using the cam.

## Materials used
- ESP32 Cam board.
- Any phone with Blynk app
- Servo motors or normal motors too but really servo motors
- Neopixel rings

## Setup
It is basically written in C, so my guess is that you can use it in other embedded systems but you won't be able to run it on any other microcontroller because the pins are setup for ESP-32 cam.

You can host your images on any of the web hosts, I used 000webhost for my images but you can use any thing else. The blynk app access the URL since it is public.

## Issues

- The image seems to get corrupted sometimes, although theoretically it shouldn't.
- I have written functions for the servo motor but they don't work right because of timer issue. The same timers are being used by the servo as well as the camera. So if you can figure out a way to fix that, it will help a lot. 

## Credits
 I basically used [this](https://github.com/ldab/ESP32-CAM-Picture-Sharing) for my [course project](https://sites.google.com/view/cics290m) and I am really grateful. I only made some minor changes to the code so the entire credits go to the owner of [this repository](https://github.com/ldab/ESP32-CAM-Picture-Sharing).
