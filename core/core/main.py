#!/usr/bin/python
# Test cmd: echo 1,1,$(date +%s).000000,1 | nc localhost 12000

import socket
import os

HOST = "0.0.0.0"  # Standard loopback interface address (localhost)
PORT = int(os.environ['CORE_PORT'])  # Port to listen on

NUM_DEVICES = int(os.environ['NUM_DEVICES'])

NUM_ALGS = 8

# import thread module
from _thread import *
import threading
import time
import datetime

import mysql.connector

start_time = time.time_ns() // 1e3

device_start_time_delta = [-1] * (NUM_DEVICES + 1) * (NUM_ALGS + 1)

message_frequency = [0] * (NUM_DEVICES + 1) * (NUM_ALGS+1)

def restartMysql():
    conn = mysql.connector.connect(
        host="mysql",
        user="user",
        password="password",
        database="core_db"
    )
    cursor = conn.cursor()
    return conn, cursor

def table_lookup(device, alg):
    return "d_" + str(device) + "_" + str(alg)

insert = '''INSERT INTO `%s` (`timestamp`, `value`, `deltatime`, `message_frequency`) VALUES ('%s', %s, %s, %s);'''

# thread function
def threaded(c):
    conn, cursor = restartMysql();
    while True:
        try: 
            raw = c.recv(1024)
            print(f"Received: {raw}")
        except:
            print("Could not receive raw data, connection close")
            break
        if not raw:
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
                print(f"Invalid format: {raw}")
                print(f"Received: {raw}")
                continue

            if(len(args) != 4):
                print(f"Missing args: {raw}")
                continue
 
            #(INT, TIME, FLOAT)

            # Check types
            device_s  = args[0]
            alg_s  = args[1]
            time_s  = args[2]
            value_s = args[3]

            if (not device_s.isdigit()):
                print(f"Invalid device: {raw}")
                continue

            if (not alg_s.isdigit()):
                print(f"Invalid alg: {raw}")
                continue
            
            # check if arg1 is a unix time with at most five decumals, lol float
            try:
                unixtime = int(time_s)
            except:
                print(f"Invalid time: {raw}")
                continue

            # check if arg2 is a float
            if (not value_s.replace('.', '', 1).lstrip('-').isdigit() or value_s.count('-') > 1 or value_s.count('.') > 1 ):
                print(f"Invalid value: {raw}")
                continue

            # check if arg0 is a valid device
            try:
                device_id = int(device_s)
                alg_id = int(alg_s)
                table = table_lookup(device_id, alg_id)
            except:
                print(f"Invalid device: {raw}")
                continue

            key = device_id * NUM_ALGS + alg_id

            current_time = int(time.time() * 1_000_000)
            delta = (current_time - unixtime) / 1000 #Go from micro to milliseconds

            start_delta = unixtime - start_time

            # fuckit, this will solve time whitout needing changes on the devices
            if (device_start_time_delta[key] == -1):
                device_start_time_delta[key] = start_delta
                unixtime = start_time
            else:
                d = device_start_time_delta[key]
                unixtime = unixtime - d

            current_message_frequency = "NULL"
            current_number_of_messages = message_frequency[key] + 1
            # skip on first 10 messages, cuz easier to manage view in Grafana
            if (current_number_of_messages >= 10):
                current_message_frequency = current_number_of_messages / start_delta
            
            message_frequency[key] = current_number_of_messages
                
            timestamp = datetime.datetime.fromtimestamp(unixtime / 1e6).strftime('%Y-%m-%d %H:%M:%S.%f')
            val = f"{float(value_s):.12f}"

            
            stmnt = (insert % (table, timestamp, val, delta, current_message_frequency))

            try:
                cursor.execute(stmnt)
                conn.commit()
            except mysql.connector.Error as e:
                print(f"Could not insert into database for statment: {stmnt}")
                print(e)
                cursor = None
                conn = None
                conn, cursor = restartMysql();
                continue
            
            print(f"Inserted: {stmnt}")        
            
    # connection closed
    c.close()


def Main():

    conn, cursor = restartMysql();
    for device in range(1, NUM_DEVICES+1):
        for alg in range(1, 9): # fuckit
            # create table if none exists
            cursor.execute(('''CREATE TABLE IF NOT EXISTS `%s` (
                `id` int NOT NULL AUTO_INCREMENT,
                `timestamp` DATETIME(6) NOT NULL,
                `value` double NOT NULL,
                `deltatime` double NOT NULL,
                `message_frequency` double NULL,
                PRIMARY KEY (`id`, `timestamp`),
                UNIQUE KEY `timestamp` (`timestamp`)
            );''' % table_lookup(device, alg)))

    conn.commit()

    cursor.close()
    conn.close()

    for res in socket.getaddrinfo(HOST, PORT, socket.AF_UNSPEC, socket.SOCK_STREAM, 0, socket.AI_PASSIVE):
        af, socktype, proto, canonname, sa = res
        try:
            s = socket.socket(af, socktype, proto)
        except socket.error as msg:
            s = None
            continue

        try:
            s.bind((HOST, PORT))
            s.listen()
        except socket.error as msg:
            s.close()
            s = None
            continue
        break
    if s is None:
        print('could not open socket')
        sys.exit(1)
    
    # establish connection with client
    c, addr = s.accept()

    print('Connected to :', addr[0], ':', addr[1])

    # Start a new thread and return its identifier
    start_new_thread(threaded, (c,))

    c.close()
    print("connection closed")

    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.bind((HOST, PORT))

    print(f"Started server on: {HOST}:{PORT}")

    # put the socket into listening mode
    s.listen() # apperently, this value is the default value: /proc/sys/net/core/somaxconn
    print("socket is listening")

    # a forever loop until client wants to exit
    while True:

        # establish connection with client
        c, addr = s.accept()

        print('Connected to :', addr[0], ':', addr[1])

        # Start a new thread and return its identifier
        start_new_thread(threaded, (c,))


if __name__ == '__main__':
    Main()
