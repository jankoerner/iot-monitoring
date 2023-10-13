#!/usr/bin/python

import socket
from _thread import *
import threading
import time
import datetime
import os

NUM_DEVICES = int(os.environ['NUM_DEVICES'])
FREQUENCY = int(os.environ['SAMPLE_FREQ']) # Hz

HOST = "0.0.0.0"  # Standard loopback interface address (localhost)
PORT = int(os.environ['SINK_PORT'])  # Port to listen on
CORE_PORT = int(os.environ['CORE_PORT'])  # Core port to connect to

frq = 1/FREQUENCY
class SIP:
    def __init__(self, k, m):
        self.k = k
        self.m = m
        self.lastSampledTime = 0
    
    def getPrediction(self, t):
        self.lastSampledTime = time.time_ns() // 1e6
        return self.k * t + self.m
    
    def getFunction(self):
        return [self.k, self.m]
    
    def setFunction(self, k, m):
        self.k = k
        self.m = m

# Spawn thread that samples the sink
def socket_thread(c):
    while True:
        try: 
            raw = c.recv(1024)
            print(f"Received: {raw}")
        except:
            print("Could not receive raw data, connection close")
            break
        if not raw: 
            print("No raw data received, connection close")
            break
        
        if(raw == b''):
            print("Empty raw data received, connection close")
            break
        
        try:
            data = raw.decode("utf-8")
        except:
            print("Could not decode raw data, connection close")
            break

        # for each new line
        for line in data.splitlines():
            try:
                args = line.split(",")
            except ValueError:
                print("Invalid data format, continue")
                continue

            if(len(args) != 4):
                print("Invalid data format, continue")
                continue
 
            #(INT, TIME, FLOAT, FLOAT)

            # Check devices
            device_s  = args[0]
            time_s  = args[1]
            k_s = args[2]
            m_s = args[3]

            if (not device_s.isdigit()):
                print("Invalid device format, continue")
                continue
            
            # check if arg1 is a unix time with at most five decumals
            try:
                unixtime = int(time_s)
            except:
                print("Invalid time format, continue")
                continue

            # check if arg2 is a float
            if (not k_s.replace('.', '', 1).lstrip('-').isdigit() or k_s.count('-') > 1 or k_s.count('.') > 1 ):
                print("Invalid data format, continue")
                
            # check if arg3 is a float
            if (not m_s.replace('.', '', 1).lstrip('-').isdigit() or m_s.count('-') > 1 or m_s.count('.') > 1 ): 
                print("Invalid data format, continue")
                continue

            # check if arg0 is a valid device
            try:
                sink = sinks[int(device_s)]
            except:
                print("Invalid not a device, continue")
                continue

            #unixtime = "{:.6f}".format(float(time_s))
            timestamp = datetime.datetime.fromtimestamp(unixtime // 1e6).strftime('%Y-%m-%d %H:%M:%S.%f')

            # print time diff unixtime from current time

            print(k_s, " ", m_s)
            k = float(k_s)
            m = float(m_s)

            sink.setFunction(k, m) 
    # connection closed
    c.close()

def sample_thread():
    time_start = time.time_ns() // 1e6
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect(("core", CORE_PORT))

    while True:
        for i in range(NUM_DEVICES):
            sink = sinks[i]

            #time_since_start = time.time_ns() - time_start
            t = int(time.time_ns() // 1e6)
            prediction = sink.getPrediction(t)

            if prediction != 0: # cheat
                print(f"Sampled: {prediction:.12f} @ {t} from device {i}")
                s.sendall(f"{i},7,{t},{prediction:.12f}\n".encode('utf-8'))

        time.sleep(frq)

sinks = [None] * NUM_DEVICES

for i in range(NUM_DEVICES):
    sinks[i] = SIP(0,0)

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.bind((HOST, PORT))

print(f"Started server on: {HOST}:{PORT}")

# Listen for incoming connections
s.listen()  # Listen for a single incoming connection

start_new_thread(sample_thread, ())

while True:
    c, addr = s.accept()
    start_new_thread(socket_thread, (c,))
