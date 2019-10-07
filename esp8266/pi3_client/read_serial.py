import serial
#from time import sleep

from datetime import datetime

# installed with =>  python3 -m pip install redis
import redis


def send_over_wifi(request):
    #print("sending:"+request)
    # we do not have enough time to handle this here. just through in redis and someone else will take care of wifi
    redis_db = redis.StrictRedis(host="localhost", port=6379, db=0)
    redis_db.rpush("mylist", request)  # rpush to add / lpop to retreive
    
def log(line):
    x = 0
    l = str(datetime.now())+"\t"+line+"\n"
    with open("/var/log/agv.log", "a") as myfile:
        myfile.write(l)


def run():
    ser = serial.Serial ("/dev/ttyAMA0", 115200)    #Open port with baud rate
    while True:
        data = ser.readline()
        try:
            data = data.decode("ascii").strip()
        except:
            data = ""
            pass
        #data = ser.readline().decode("ascii").strip()
        if data.startswith("?"):
            send_over_wifi(data)
        elif "\t" in data:
            log(data)
            


if __name__ == "__main__":
    run()