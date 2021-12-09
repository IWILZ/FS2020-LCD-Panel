## **Would you contribute?**
You can do it in two ways😊

### **The PC side**
In this project i used the good FS2020TA.exe but this program has a little limit.
Every parameter is sent ciclically and not when Arduino needs it and this unfortunately produces a **waste of time**.

The best solution would be that Arduino can **"ask" for a specific value only when it needs**.
But this requires a different and **new interface on the PC side between the MCU and FS**.
So if you want to make it, you are welcome!

### **A new multiprocessor architecture**
What if we could have a multiprocessor/multifunction panel?

Imagine how nice it could be to have a number of dedicated panels that perform various specific functions such as aircraft control management, radio stack, autopilot, GPS, etc.
All these panels must communicate with FS requesting their data and sending commands.
You may then need **an Arduino board which should only be used for communicating with the PC and delivering data to the other boards**.

It could be a good milestone! 
