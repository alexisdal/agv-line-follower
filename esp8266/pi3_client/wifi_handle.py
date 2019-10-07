# this Python file uses the following encoding: utf-8

import os # to execute shall cmds
import time # to sleep()
import redis # to interface with the queue of requests. installed with =>  python3 -m pip install redis

import requests # because if's simpler to use for an http query than the default python . installed with python3 -m pip install requests

#RSSI=$(grep wlan0 /proc/net/wireless | awk '{print $4}' | tr -d '.')
#root@mtbr00622:~# cat /proc/net/wireless
#Inter-| sta-|   Quality        |   Discarded packets               | Missed | WE
# face | tus | link level noise |  nwid  crypt   frag  retry   misc | beacon | 22
# wlan0: 0000   49.  -61.  -256        0      0      0      0      0        0
    
def get_wifi_rssi():
    cmd = "cat /proc/net/wireless | grep wlan0"
    res = os.popen(cmd).read().strip()
    # wlan0: 0000   49.  -61.  -256        0      0      0      0      0        0
    with_single_space = " ".join(res.split())
    #print(with_single_space)
    # wlan0: 0000 48. -62. -256 0 0 0 0 0 0
    parts = with_single_space.split(' ')
    #print(str(parts))
    # ['wlan0:', '0000', '48.', '-62.', '-256', '0', '0', '0', '0', '0', '0']
    return parts[3] if len(parts)>=3 else "0"


#CHAN=$(iwlist wlan0 channel | grep Current | rev | tr -d ')' | awk '{print $1}' | rev)
#root@mtbr00622:~# iwlist wlan0 channel | grep Current
#          Current Frequency:5.18 GHz (Channel 36)

def get_wifi_channel():
    cmd = "iwlist wlan0 channel | grep Current | rev | tr -d ')' | awk '{print $1}' | rev"
    res = os.popen(cmd).read().strip()
    return res

def get_queue_len():
    redis_db = redis.StrictRedis(host="localhost", port=6379, db=0)
    res = redis_db.llen("mylist")  # get mylist len
    # print(str(type(res))) => <class 'int'>
    return res
    

def run():
    while(True):
        redis_db = redis.StrictRedis(host="localhost", port=6379, db=0)
        queue_size = redis_db.llen("mylist")  # get mylist len
        if (queue_size <= 0):
            time.sleep(1.0)
        else :
            request = redis_db.lindex("mylist", 0).decode("ascii")
            #print(request)
            #?n=AGV_DEV&v=24.74&tc=-2&dc=0.00&ct=4772639&sl=5734303432371437&fw=0.9.5.2&km=47&m=173.24&nb=0&nc=0&ne=0&ll=0
            rssi = get_wifi_rssi()
            #print(rssi)
            channel = get_wifi_channel()
            #print(channel)
            url = "http://10.155.249.179/cgi-bin/insert.py"+request+"&rssi="+rssi+"&channel="+channel+"&queue_size="+str(queue_size)
            #print(url)
            try:
                r = requests.get(url)
                #print(r.status_code)
                #print(r.text)
                
                if (r.status_code == 200 and r.text.lower().startswith("ok")):
                    # dequeue the current request
                    redis_db.lpop("mylist")
            except:
                pass
            time.sleep(1.0)
    
if __name__ == "__main__":
    run()