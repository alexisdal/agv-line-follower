import serial
from time import sleep

ser = serial.Serial ("/dev/serial1", 115200)    #Open port with baud rate
while True:
    data = ser.readline()
    sleep(0.3)
    print(data)
