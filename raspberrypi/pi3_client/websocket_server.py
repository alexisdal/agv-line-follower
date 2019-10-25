#!/usr/bin/env python

# inspired from 
# https://websockets.readthedocs.io/en/stable/intro.html
# https://github.com/aaugustin/websockets/issues/39


import asyncio
import websockets
#from datetime import datetime

import logging
logger = logging.getLogger('websockets')
logger.setLevel(logging.INFO)
logger.addHandler(logging.StreamHandler())


import redis

import json

import time

#root@mtba00000:~# redis-cli llen mycmds
#(integer) 69
#root@mtba00000:~# redis-cli lrange mycmds 0 -1
# 1) "drive:1"
# 2) "xy:-1|-12"
# 3) "xy:-2|-22"
# 4) "xy:-2|-22"
# 5) "xy:-4|-35"
# 6) "xy:-7|-56"
# 7) "xy:-7|-56"
#root@mtba00000:~# redis-cli del mycmds
#(integer) 1
#root@mtba00000:~# redis-cli llen mycmds
#(integer) 0
#root@mtba00000:~#

def cap(x,limit):
    if x < -limit:
        return -limit
    if x > limit:
        return limit
    return x


# reminder of left/right logic
#  float factor = (float)linePosition / (pixy.frameWidth / 2);
#  //KSTEEP = (nominal_speed - ESC_STOP) * 1.025f; // how it should have been
#  KSTEEP = (nominal_speed - ESC_STOP);
#  // now adjust left/right wheel speeed
#  if (linePosition == 0)
#  {
#    motor_speed_left  = nominal_speed;
#    motor_speed_right = nominal_speed;
#  }
#
#  if (linePosition > 0)//turn right
#  {
#    motor_speed_left  = nominal_speed;
#    motor_speed_right = nominal_speed - KSTEEP * factor;
#  }
#
#  if (linePosition < 0)//turn left
#  {
#    motor_speed_left  = nominal_speed - KSTEEP * (-1) * factor;
#    motor_speed_right = nominal_speed;
#  }

def joy_xy_to_motor_cmd(x,y):
    ESC_MIN = -5000
    ESC_MAX = +5000
    ESC_STOP = 150
    joy_limit = 150
    x = cap(x, joy_limit)
    y = cap(y, joy_limit)
    
    speed =  ESC_MAX * y / joy_limit;
    KSTEEP = (speed - ESC_STOP);
    factor = float(x)/joy_limit
    if factor<0: # left
        motor_speed_left  = int(speed - KSTEEP * (-1) * factor   )
        motor_speed_right = int(speed                            ) 
    else:  # right
        motor_speed_left  = int(speed                            )
        motor_speed_right = int(speed - KSTEEP * (+1) * factor   )
    
    return (motor_speed_left , motor_speed_right)



async def handler(websocket, path):
    print("'"+(path)+"'")
    redis_db = redis.StrictRedis(host="localhost", port=6379, db=0)
    
    while True:
        if not websocket.open:
            break
        
        if (path == "/manual_drive"):
            msg = ""
            try:
                msg = await websocket.recv()
            except:
                pass
            #print(msg)
            if msg.startswith("xy"):
                # ok... JSON would be more elegant :/
                #d = datetime.now().isoformat().replace("T", " ")
                ## xy:-77|-261
                xy, values = msg.split(":")
                x, y = values.split("|")
                x = int(x)
                y = int(y)
                #print(d+"\txy\t"+x+"\t"+y)
                (motor_speed_left , motor_speed_right) = joy_xy_to_motor_cmd(x, y)
                cmd = f"M L{motor_speed_left}  R{motor_speed_right}"
                #print(cmd)
                redis_db.rpush("mycmds", cmd)  # rpush to add / lpop to retreive
                # useful tool => https://jsoneditoronline.org/
                j = f'{{ "l":{motor_speed_left} , "r":{motor_speed_right} }}'
                #print(j)
                #print("")
                await websocket.send(j)
            elif msg.startswith("drive"):
                #drive:0
                #drive:1
                cmd = msg.replace("drive:", "D")
                redis_db.rpush("mycmds", cmd)  # rpush to add / lpop to retreive
            else:
                print("unknown msg: "+msg)
        elif (path == "/live_data"):
            if (redis_db.llen("mylogs") > 0):
                lines = []
                for i in range(0, 10):
                    line = redis_db.lpop("mylogs")
                    if line != None:
                        line = line.decode("ascii")
                        if not "e" in line:
                            r = line.split("\t")
                            #lines.append(line)
                            lines.append( [ int(r[0]), int(r[1]) ] )
                try:
                    await websocket.send(json.dumps(lines))
                except:
                    pass
                #await websocket.send(json.dumps(lines))
            else:
                time.sleep(1.0)
        elif (path == "/pid_tuning"):
            msg = ""
            try:
                msg = await websocket.recv()
                print(msg)
                j = json.loads(msg);
    
                cmd = f"P P{j['Kp']} I{j['Ki']} D{j['Kd']}"
                #print(cmd)
                redis_db.rpush("mycmds", cmd)  # rpush to add / lpop to retreive
            except:
                pass
        else:
            print("'"+(path)+"'")
        

            
def main():
    print("starting server on port 3000")
    server = websockets.serve(handler, "0.0.0.0", 3000)
    
    try:
        asyncio.get_event_loop().run_until_complete(server)
        asyncio.get_event_loop().run_forever()
    except KeyboardInterrupt:
        print('\nCtrl-C (SIGINT) caught. Exiting...')
    finally:
        #dont care
        #asyncio.get_event_loop().close()
        #server.close()
        x = 0 
        
if __name__ == "__main__":
    main()