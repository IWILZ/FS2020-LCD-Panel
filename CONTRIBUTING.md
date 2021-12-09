##Would you contribute?
You can do it in two ways😊

###The PC side
In this project i used FS2020TA.exe but this program gas a Little limite.
Every parameter is sent ciclically and not when Arduino needs it.
This unfortunately can produce a waste of time.

The best solution would be that Arduino can "ask" for a specific value only when it really needs it.
But this needs a different and new interface on the PC between the MCU and FS.
If you want to make it, you are welcome!

###A new multiprocessor architecture
What if we could have a multiprocessor/multifunction panel?

Imagine how could be nice if we could have more dedicated panels doing specific functions such as aircraft commands, radio stack, autopilot, GPS, etc

All those panels needs to communicate with FS requesting and sending some data/commands.
So could be necessary another Arduino board who should be used only for the communication and data delivery to the other boards.

This could be a nice goal!
