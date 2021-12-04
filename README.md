# FS2020-LCD-Panel with Arduino
Two alphanumeric LCDs and 2 LED bars to show and manage some in-flight parameters for FS2020. In this project you can play with some Arduino programming code but you don't need any development effort on the PC side. 

![picture 1](https://user-images.githubusercontent.com/94467184/144686894-c6397e9b-c9e7-4ae6-ac98-a33d2978ecd4.jpg)

About the radio stack and due to the large number of planes with very different functionalities (for example, there are planes that have the standby frequency on the ADF and those that don't), it could be necessary to customize the program to obtain some function on different planes. 

## **Why this project**
This second project is a bit more complicated than the first **Switch/LED Panel** but anyway i will give you every useful informations to let you build your own panel.

Again i started with these main goals in mind:
1. i wanted to build it using one of my Arduino(s) i had in my drawer
2. i wanted to write some code for it so i didn't chose any "high-level" developing HW/SW platform (like Mobiflight for example). They are very powerful, but **i like to write my own code and solder some wires from scratch to obtain a flexible solution customized on my needs**
3. i wanted to realize a simple, flexible and cheap panel

Like my first project, you can use **an Arduino board of any type** (i used a Nano and the only limit is the number of I/O pins) plus some cheap other stuff like an encoder, a couple of LCDs and few LEDs and buttons.

## **What it does**
This panel manages and shows the following parameters:
1. display **NAV1 & NAV2 Active** frequencies
2. display **NAV1 & NAV2 Standby** frequencies
3. edit and switch NAV1 & NAV2 Standby frequencies
4. display and edit **radial setting** of NAV1 & NAV2 (**OBS**)
5. display and edit **ADF frequency and its HDG**
6. when a NAVx starts to receive a valid signal, the program automatically displays its **CDI** on 2 LED bars (with priority to NAV1).
7. on a second LCD the program displays **Indicated Air Speed, Altitude (QNH), Height (QFE), HDG and Vertical Speed**

I made my panel to be placed above my PC monitor so i can see all the parameters in the most comfortable way.

All the Radio frequencies and courses are shown and edited on the first 16x2 char LCD. Some plane parameter like Speed(s), Altitude, HDG, etc, are shown on a second LCD and the CDI is shown using 2 LED bars + a single 3mm green central LED.
To edit each frequency and course i used an encoder + 1 button.

<img src="https://user-images.githubusercontent.com/94467184/144687184-f3bc54db-0105-41a0-8961-6876fe6753f5.jpg" width="70%" height="70%">


## **The architecture**
In the following picture you can see **3 components**:

1. **the LCD panel** (with Arduino and some buttons and LEDs)
2. a program (**FS2020TA.exe**) that manages the bidirectional communications with FS (you will find a link to this program later in this document)
3. the **Flight Simulator** itself
The panel reads values and send commands to FS using the **FS2020TA.exe** (made by Seahawk240) as a sort of communication "repeater". The Arduino board communicates with the PC and FS2020TA.exe (that uses a SimConnect.dll) using a standard USB port. The communication protocol is very simple and will be explaned later.
<img src="https://user-images.githubusercontent.com/94467184/144688653-83b6088f-c166-4de2-abef-9cad8941791a.jpg" width="70%" height="70%">

## **What you need**
What you need is:

1. an ordinary Arduino board (not necessarily a Nano)
2. two LDC 16x2 chars
3. two led bars 
4. one encoder with push button
5. a couple of buttons or spring switches
6. some LEDs and their resistors
7. a couple of small breadboards
8. a soldering iron
9. some small section wires to connect switches and LEDs to the Arduino board

![picture 3](https://user-images.githubusercontent.com/94467184/144688952-f0fa672a-c81d-4e6d-84a0-019ed829cc10.jpg)

For the front panel i used a piece of a carbon fiber plate but you can use also a wood plate or anything else from about 1.5 to 3mm thick.
Due to the small power consumption, **the panel will be simply powered by the 5Vcc from the USB connection of your PC**.

## **LEDs and buttons connections**
Before to start is better to spend few words about LEDs and buttons connection.

Each Arduino pin can be configured to be an Input or Output by the program so all it's very flexible. Obviously every LED connection has to be an Output (any output pin produces a 5Vcc when at high level) and every button/switch as Input but in this last case the program have to configure it like an **"INPUT_PULLUP"** pin to avoid random readings.

In the following picture you can see how to connect a generic LED and a generic button/switch.

<img src="https://user-images.githubusercontent.com/94467184/144689525-0ceccac7-b2d9-435a-b5f5-9f292a1aac2a.jpg" width="80%" height="80%">

**IMPORTANT**: to avoid a damage of the micro controller itself **NEVER CONNECT A LED DIRECTLY to the Arduino**, but use a **resistor** to limit the current flowing to the LED. **The resistor value depends on the LED brand and colour** (normally red ones needs a lower value resistor than green ones) but you could start with a value of 1KOhm and then change it to find the right value/light for your LED. If you have a tester you can also measure the current flowing into the LED considering that the maximum current on a output PIN of the Arduino cannot **never exceed 20mA**. If you cannot measure the current, just look at your LED's light and don't exceed with its brightness.

Each button should be "normally opened" so it will "close the circuit" to the ground only when pressed.

## **The Encoder**
In this project the encoder allow us to edit frequencies and courses rotating the shaft so it is quite a sort of "fast button" that changes its state very quickly when is turned right or left. Furthermore, our encoder must have also a button that is activated by pressing the rotation shaft. For these reasons an encoder is a bit more complex than a simple button as you can see in the following picture but this is not a problem because the program uses the **BasicEncoder library** to manage it.

<img src="https://user-images.githubusercontent.com/94467184/144690024-96a5ee97-b932-485a-949e-6504f55dfdcd.jpg" width="30%" height="30%">

## **The LCD and the bus I2C**
Also these devices are more complex than a simple LED but 2x16 chars LCDs are widely used in a lot of projects so they are cheap and well known. They are available in 2 versions: **with** and **without** a serial interface and in this project **we need the first version** because they need **only 2 PINs** (in addition to power supply) to be connected to an Arduino board.

The serial interface version of the LCD comes with a small additional board (that must be soldered to the LCD one) that allows to communicate with other devices using a **serial protocol called I2C**.

<img src="https://user-images.githubusercontent.com/94467184/144691093-544582cf-7d3e-490a-b2f9-4e852264e4de.jpg" width="35%" height="35%">

So, within the project, the modules communicate with each other according to the scheme of the following figure and the communications will be manged by the **LiquidCrystal_I2C library**.

<img src="https://user-images.githubusercontent.com/94467184/144692377-9d0b1b67-9b64-42e5-a704-afd23688af9c.jpg" width="70%" height="70%">

## **LED bars and CDI**
The last components i've used are a couple LED bars (+ 1 green central LED) to realize a CDI (Course Deviation Indicator) when we are in range with a VOR.
Each bar has 10 individually driven LEDs. Eight LEDs are green, one is yellow and the last is red. Using these bars i can see how much deviation (left or right) is from the radial set via the OBS on a specific VOR station. Also in this case the simple **Grove_LED_Bar library** is used to manage the bars. 


<img src="https://user-images.githubusercontent.com/94467184/144693314-ae652585-4712-4aac-8cbd-42745784322a.png" width="30%" height="30%">




**Sorry.... still under construction**
