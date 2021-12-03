# FS2020-LCD-Panel
Two alphanumeric LCDs and 2 LED bars to show and manage some in-flight parameters for FS2020.

## **Why this project**
This my second project is a bit more complicated but anyway (if you will like it) i will try to give you every useful informations to let you build your own panel.
Also in this case i've started with these main goals in my mind:
1. i wanted to build it using one of my Arduino(s) i had in my drawer
2. i wanted to write some code for it so i didn't chose any "high-level" developing HW/SW platform (like Mobiflight for example). They are very powerful, but i like to write my own code and solder some wires from scratch
3. i wanted to realize a simple, flexible and cheap panel too

Like my first project, this panel don't need any development effort on the PC side and you can use **an Arduino board of any type** (i used a Nano) plus some cheap other stuff like an encoder, a couple of LCDs and few LEDs and buttons.

## **What it does**
This panel manages and shows some parameters like the following:
1. display NAV1 & NAV2 Active frequencies
2. display NAV1 & NAV2 Standby frequencies
3. edit and switch NAV1 & NAV2 Standby frequencies
4. display and edit radial setting of NAV1 & NAV2 (OBS)
5. display and edit ADF frequency and HDG
6. when a NAVx is tuned to an ILS, automatically display its CDI on 2 LED bars
7. display Indicated Air Speed, Altitude (QNH), Height (QFE), HDG, Vertical Speed

