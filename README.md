# FS2020-LCD-Panel with Arduino
Two alphanumeric LCDs and 2 LED bars to show and manage some in-flight parameters for FS2020.
![picture 1](https://user-images.githubusercontent.com/94467184/144686894-c6397e9b-c9e7-4ae6-ac98-a33d2978ecd4.jpg)

## **Why this project**
This my second project is a bit more complicated but anyway i will try to give you every useful informations to let you build your own panel.

Again i started with these main goals in mind:
1. i wanted to build it using one of my Arduino(s) i had in my drawer
2. i wanted to write some code for it so i didn't chose any "high-level" developing HW/SW platform (like Mobiflight for example). They are very powerful, but i like to write my own code and solder some wires from scratch
3. i wanted to realize a simple, flexible and cheap panel

Like for my first project, this panel don't need any development effort on the PC side and you can use **an Arduino board of any type** (i used a Nano) plus some cheap other stuff like an encoder, a couple of LCDs and few LEDs and buttons.

## **What it does**
This panel manages and shows some parameters like the following:
1. display NAV1 & NAV2 Active frequencies
2. display NAV1 & NAV2 Standby frequencies
3. edit and switch NAV1 & NAV2 Standby frequencies
4. display and edit radial setting of NAV1 & NAV2 (OBS)
5. display and edit ADF frequency and HDG
6. when a NAVx is tuned to an ILS, automatically display its CDI on 2 LED bars
7. display Indicated Air Speed, Altitude (QNH), Height (QFE), HDG, Vertical Speed

I made my panel to be placed above my PC monitor so i can see all the parameters in the most comfortable way.

All the Radio frequencies and courses are managed on a first 16x2 char LCD. Speed(s), Altitude, HDG, etc are shown on a second LCD and the CDI is shown using 2 LED bars + a single 3mm green central LED.
To edit every frequency and course i used an encoder + 1 button.
![picture 0](https://user-images.githubusercontent.com/94467184/144687184-f3bc54db-0105-41a0-8961-6876fe6753f5.jpg)


## **The architecture**

**Sorry.... Under construction**
