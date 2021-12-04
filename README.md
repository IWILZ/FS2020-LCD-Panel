# FS2020-LCD-Panel with Arduino
Two alphanumeric LCDs and 2 LED bars to show and manage some in-flight parameters for FS2020.
![picture 1](https://user-images.githubusercontent.com/94467184/144686894-c6397e9b-c9e7-4ae6-ac98-a33d2978ecd4.jpg)
In this project you can play with some Arduino programming code but you don't need any development effort on the PC side. 

## **Why this project**
This second project is a bit more complicated than the first **Switch/LED Panel** but anyway i will try to give you every useful informations to let you build your own panel.

Again i started with these main goals in mind:
1. i wanted to build it using one of my Arduino(s) i had in my drawer
2. i wanted to write some code for it so i didn't chose any "high-level" developing HW/SW platform (like Mobiflight for example). They are very powerful, but i like to write my own code and solder some wires from scratch
3. i wanted to realize a simple, flexible and cheap panel

Like my first project, you can use **an Arduino board of any type** (i used a Nano and the only limit is the number of I/O pins) plus some cheap other stuff like an encoder, a couple of LCDs and few LEDs and buttons.

## **What it does**
This panel manages and shows the following parameters:
1. display NAV1 & NAV2 Active frequencies
2. display NAV1 & NAV2 Standby frequencies
3. edit and switch NAV1 & NAV2 Standby frequencies
4. display and edit radial setting of NAV1 & NAV2 (OBS)
5. display and edit ADF frequency and its HDG
6. when a NAVx starts to receive a valid signal, the program automatically displays its CDI on 2 LED bars (with priority to NAV1).
7. on a second LCD the program displays Indicated Air Speed, Altitude (QNH), Height (QFE), HDG and Vertical Speed

I made my panel to be placed above my PC monitor so i can see all the parameters in the most comfortable way.

All the Radio frequencies and courses are shown and edited on the first 16x2 char LCD. Speed(s), Altitude, HDG, etc are shown on a second LCD and the CDI is shown using 2 LED bars + a single 3mm green central LED.
To edit every frequency and course i used an encoder + 1 button.
![picture 0](https://user-images.githubusercontent.com/94467184/144687184-f3bc54db-0105-41a0-8961-6876fe6753f5.jpg)


## **The architecture**
In the following picture you can see **3 components**:

1. **the LCD panel** (with Arduino and some buttons and LEDs)
2. a program (**FS2020TA.exe**) that manages the bidirectional communications with FS (you will find a link to this program later in this document)
3. the **Flight Simulator** itself
The panel reads values and send commands to FS using the **FS2020TA.exe** (made by Seahawk240) as a sort of communication "repeater". The Arduino board communicates with the PC and FS2020TA.exe (that uses a SimConnect.dll) using a standard USB port. The communication protocol is very simple and will be explaned later.
![picture 2](https://user-images.githubusercontent.com/94467184/144688653-83b6088f-c166-4de2-abef-9cad8941791a.jpg)

## **What you need**
What you need is:

1. an ordinary Arduino board (not necessarily a Nano)
2. two LDC 16x2 chars
3. two led bars 
4. one encoder
5. a couple of buttons or spring switches
6. some LEDs and their resistors
7. a couple of small breadboards
8. a soldering iron
9. some small section wires to connect switches and LEDs to the Arduino board

![picture 3](https://user-images.githubusercontent.com/94467184/144688952-f0fa672a-c81d-4e6d-84a0-019ed829cc10.jpg)

For the front panel i used a piece of a carbon fiber plate but you can use also a wood plate or anything else from about 1.5 to 3mm thick.
Due to the small power consumption, **the panel will be simply powered by the 5Vcc from the USB connection of your PC**.

**Sorry.... Under construction**
