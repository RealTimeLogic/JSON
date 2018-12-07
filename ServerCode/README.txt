
IoT.zip includes the server side code for the m2m-led.c example program.

The code is designed for the Mako Server: https://makoserver.net/

You may connect m2m-led.c to your own server as follows:
1: edit m2m-led.c and set the address macro to "localhost"
2: Compile the m2mled example
3: Download and unpack the Mako Server in any directory
4: Start the server example as follows: path2mako/mako -lIoT::IoT.zip
5: Start m2mled

