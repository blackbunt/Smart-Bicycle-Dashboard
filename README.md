# Smart-Bicycle-Dashboard
Smart Bicycle Dashboard based on an ESP32

With this little project you can upgrade your normal Bicycle into a smart one.
The smart dashboard shows the current speed, the ambient brightness and controls your lights depending on the ambient brightness.

A special feature is the integrated energy management:

As long as the bike is not moving, the ESP32 is in deep sleep.
When you start riding, it wakes up and does its job.
If you take a break, the ESP32 will notice and power everything off for you.
Thanks to the parking light function of the bicycle lighting, no unnecessary energy is consumed during a longer traffic light phase and you are still noticed by others!




![Cicuit](https://github.com/blackbunt/Smart-Bicycle-Dashboard/blob/master/Bicycle%20Smart%20Dashboard_Steckplatine_with_text.png)

The red LED is a placeholder for your lightning system.


The project was developed during the Informatik 1 Laboratory at the University of Applied Sciences Karlsruhe.
