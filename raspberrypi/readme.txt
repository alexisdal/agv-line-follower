this board is in charge of wifi communication.

before 0.9.5.3 we used to work an an ESP8266
but starting with 0.9.5.3 we replaced it for a raspberry pi 3b+

mega_exemple/    => something to test/debug the serial connection (will likely be deleted in the future)

pi3_client/      => code to load in the pi 3b+ 
pi3_client/*.*   => should go in  /root
pi3_client/html/ => should go in /var/www/html/ to be served by apache2
stats_server/    => should go in the distant server that collects data from all AGVs (vmware)


*************************


why swapping ESP8266 for raspberrypi 3B+?

initiatlly, I wanted something super simple/stupid (think KISS), where there wouldn't be an entire linux with millions lines of code of dependancies where we do not really know/understand some behavior and cannot determine if it's caused by the underlying OS or not. Which is why we used "dumb" ESP8266 to just take url from the main.ino over serial and send them over wifi though a simple HTTP request to our stats_server.

However, in production on the pilot site, we observed a saturated/polluted environment on 2.4GHz which caused several requests to never reach the stats_server (up to 30-40% of missing requessts). To workaround the limitation, we simply used a logic of queueing the events in the ESP8266 and send them all over wifi whenever the wifi window would re-open. that made the dropped request rate go back to a much more acceptable number (about 3%, many of these would be caused by user turning off/on the AGV, thus flusihing the queue without sending it). But we simply want to have a working wifi because we want real-time traffic control over wifi later anyway.

We couldn't find a cheap and ubiquitous Wifi 5GHz microcontroller board like the ESP8266 is in the 2.4GHz world. Raspberry pi 3b+ and its support of 2.4/5GHz was the obvious choice.

In order to validate that, we put 3b+ on a powerbanck inside one AGV over entire days with  simple wget.sh loop that would routinely send same url to stats server (without any queue whatsoever). We observed zero loss. Surprisingly, we tried with a raspberry pi zero W (which only supports 2.4GHz, not 5GHz) for its compactness and supposedly lower power consumption compared to 3B+. We also observed a zero request loss. That was the last nail in the ESP8266 coffin.
i measured 200mA on the pi3b+ during normal operations
and about 80mA on the ESP8266.
So we nearly 3x the power consumption just to use a "decent" wifi. It can clearly be observed in the battery stats.
I did not measre the zero W power consumption. But i opted for 5Ghz for a future-proof wifi. Beside, google suggests the pi zero W has a similar power footprint anyway (https://raspberrypi.stackexchange.com/questions/63519/power-consumption-of-pi-zero-w)

And since, I then had a complete linux machine, I used the opportunity to develop the diagnostics pipeline to display live events over websocket and implement manual drive over a reative web page that nicely works even an an old iphone 4.

I did not have a ready-to-use raspberry pi 4 to compare power consumptions, but word on the street back then was that there were still GPIO issues on the board + it needs a more powerfull power supply => i dropped the idea without testing :/


