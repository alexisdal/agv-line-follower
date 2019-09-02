https://f-leb.developpez.com/tutoriels/arduino/esp8266/debuter/
https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/readme.html

file/preference add boards url :
(Arduino GUI > File > Preferences > Settings > Additionnal Board Manager
http://arduino.esp8266.com/stable/package_esp8266com_index.json

Tools > Board > Board Manager > Manage boards
in search windows: ESP8266
install esp8266 by ESP8266 Community (tested with 2.5.2) > Install

then to flash as any board, just USB plug it, choose Tools > Boards > NodeMCU 1.0
(then just choose the USB port and dont touch other settings)
    Why NodeMUC 1.0 ? because the seller says so:   
    https://www.amazon.fr/AZ-Delivery-NodeMCU-ESP8266-d%C3%A9veloppement-development/dp/B074Q27ZBQ/ref=sr_1_4?__mk_fr_FR=%C3%85M%C3%85%C5%BD%C3%95%C3%91&crid=293OHTY3QFWZF&keywords=nodemcu+esp8266&qid=1557152704&s=gateway&sprefix=node+mcu+%2Caps%2C143&sr=8-4

compiling is a bit longer than a regular arduino board



..........


<⸮⸮⸮⸮s⸮c⸮#⸮⸮ng⸮lgn⸮⸮⸮cp⸮⸮${$sdp⸮g⸮⸮l⸮⸮cn⸮|l⸮⸮c⸮⸮g'⸮
l⸮⸮dl⸮⸮d`⸮⸮'⸮start wifi
11:00:18.737 -> 
11:00:18.737 -> MAC: CC:50:E3:3C:1D:2E


ets Jan  8 2013,rst cause:2, boot mode:(3,6)

load 0x4010f000, len 1384, room 16 
tail 8
chksum 0x2d
csum 0x2d
v8b899c12
10:58:49.162 -> ~ld
10:58:49.230 -> ⸮=ePp⸮	⸮E⸮R⸮⸮J#)



****************
11:30:59.386 -> 
11:30:59.386 ->  ets Jan  8 2013,rst cause:2, boot mode:(3,6)
11:30:59.386 -> 
11:30:59.386 -> load 0x4010f000, len 1384, room 16 
11:30:59.420 -> tail 8
11:30:59.420 -> chksum 0x2d
11:30:59.420 -> csum 0x2d
11:30:59.420 -> v8b899c12
11:30:59.420 -> ~ld
11:30:59.454 -> ⸮⸮5m
11:30:59.454 -> ⸮	⸮3Z⸮⸮JX;N⸮

