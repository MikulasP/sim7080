# SIM7080
A work in progress, Arduino compatible library for SIMCOM SIM7080G.

##Documents for the SIM7080G module

The official documentations are available on the simcom website: https://www.simcom.com/product/SIM7080G.html

## About this library

This library is part of a larger project where an Arduino compatible SIM7080 library was required. 
As of now (May 2023) the time schedule is tight and because of that functionalities are not yet implemented.

Updates with new functions and optimizations are to be expected soon.

##Some notes

During the development I ran into a few problems that are worth mentioning:

* The device can operate the GNSS submodule and connect to the mobile network. However, communicating over 4G would not work until the GNSS submodule is running. (I couldn't find anything about the GNSS submodule and the 4G communication in the documentations, that would suggest that the two functions cannot be used at the same time.) For now it is recommended that the GNSS submodule ang the 4G communications are kept apart.
* Regarding the above point after disabling the GNSS submodule the chip has a hard time actually communicating over the 4G network. After powering down the GNSS and activating the APP network it takes between 1 and ~5 minutes to be able to send and receive data on the network, despite the device getting an IP address right at the APP network activation. Rebooting the module after powering down the GNSS seems to solve this problem for the 4G network comms.
* For some reason the HTTP(S) functionality is unavailable. Despite following the official documentation on HTTP(S) setup and operation the module gives "Operation not allowed" error every time. I found multiple people having this issue, but to my knowledge there is no solution known to this problem. (May 2023)

If you have any advice or additional information regarding this module, I would warmly welcome them. :)

Thank you for checking out this repo!

   --Miku
