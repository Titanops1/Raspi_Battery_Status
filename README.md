# pi-top-battery-status (version 1.1)

- Displays the battery status of the pi-top battery
(a laptop based on the raspberry pi, see http://pi-top.com)
- Gives a warning if capacity is less or equal to 15%
- Shuts system down if capacity is less or equal to 10%, even if user pi does not respond to warnings
- Logs all activities in /home/pi/batteryLog.txt

![Alt text](screenshot.jpg?raw=true "battery charge")

The battery status is displayed on the desktop panel.
**You must be user pi to install and use the program.**
Make sure that i2c is enabled in raspi-config.  

To install:

- Download the repository to your pi-top using the "download zip" button
- Using the file manager, go to your download folder,
 right click on "pi-top-battery-status-master.zip" and choose "Extract here"
- Open a console window and type the following commands

```
  cd Downloads
  cd pi-top-battery-status-master
  chmod +x ./install
  ./install
```
- Reboot your pi

If you are running standard raspian instead of pi-top-os, and you get a 0% battery level display,
you might have to turn i2c on in raspi-config and install i2c-tools with the command

```
  sudo apt-get install i2c-tools
```

If you want to recompile the program, you need to install

```
  sudo apt-get install libgtk-3-dev
```

You can test the compilation with

```
  touch gtk_battery.c
  make
```

If you want to change the behaviour of gtk_battery 
(no log file, other warning and shutdown capacity limits),
see line 44 - 46 in gtk_battery.c

To uninstall this program, edit /home/pi/.config/lxsession/LXDE-pi/autostart (remove line calling gtk_battery)

Release history:

- Version 1.0: First stable release
- Version 1.0a: Fixed a minor bug, which could show a battery charge > 100% under rare circumstances
- Version 1.1: Improved reliability of capacity reading. Possibility to abort automatic low battery shutdown added.

Please help to improve this program by tweeting to
**http://twitter.com/r_richarz** or opening an issue on this repository
if you have any problem or suggestion.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details. The owner of this
repository is not affiliated with pi-top.

*Other programs to display the pi-top-battery:*

- If you prefer to display the battery gauche on a Pimorini Unicorn hat
https://shop.pimoroni.com/products/unicorn-hat
instead of the desktop panel, Jez Shed has written a python script, see
https://github.com/JezShed/PiTop-Unicorn-Battery-Gauge
