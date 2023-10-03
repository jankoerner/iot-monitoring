#!/usr/bin/python

import socket
import os

HOST = "0.0.0.0"  # Standard loopback interface address (localhost)
PORT = int(os.environ['CORE_PORT'])  # Port to listen on

NUM_DEVICES = int(os.environ['NUM_DEVICES'])

# import thread module
from _thread import *
import threading
import time
import datetime

import mysql.connector


def restartMysql():
    conn = mysql.connector.connect(
        host="mysql",
        user="user",
        password="password",
        database="core_db"
    )
    cursor = conn.cursor()
    return conn, cursor

def table_lookup(num):
    return "table_" + str(num)

conn, cursor = restartMysql();

insert = '''INSERT INTO `%s` (`timestamp`, `value`) VALUES ('%s', %s);'''

start_time = time.time()

# thread function
def threaded(c):
    while True:
        try: 
            raw = c.recv(1024)
            #print(f"Received: {raw}")
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
                print("Invalid data format, continue")
                continue

            if(len(args) != 3):
                print("Invalid data format, continue")
                continue
 
            #(INT, TIME, FLOAT)

            # Check types
            type_s  = args[0]
            time_s  = args[1]
            value_s = args[2]

            if (not type_s.isdigit()):
                print("Invalid type format, continue")
                continue
            
            # check if arg1 is a unix time with at most five decumals
            try:
                unixtime = float(time_s)
            except:
                print("Invalid time format, continue")
                continue
            if (len(time_s.split(".")) == 2):
                if(len(time_s.split(".")[1]) > 6):
                    print("Invalid time format, continue")
                    continue

            # check if arg2 is a float
            if (not value_s.replace('.', '', 1).isdigit()): 
                print("Invalid data format, continue")
                continue

            # check if arg0 is a valid type
            try:
                table = table_lookup(int(type_s))
            except:
                print("Invalid not a type, continue")
                continue

            #unixtime = "{:.6f}".format(float(time_s))
            timestamp = datetime.datetime.fromtimestamp(unixtime).strftime('%Y-%m-%d %H:%M:%S.%f')
            val = f"{float(value_s):.12f}"

            
            stmnt = (insert % (table, timestamp, val))

            try:
                cursor.execute(stmnt)
                conn.commit()
            except:
                print(f"Could not insert into database for statment: {stmnt}")
                cursor = None
                conn = None
                conn, cursor = restartMysql();
                continue
            
            print(f"Inserted: {val} into {table} @ {timestamp}")        
            
    # connection closed
    c.close()


def Main():

    for device in range(NUM_DEVICES):
        # create table if none exists
        cursor.execute(('''CREATE TABLE IF NOT EXISTS `%s` (
            `id` int NOT NULL AUTO_INCREMENT,
            `timestamp` DATETIME(6) NOT NULL,
            `value` double NOT NULL,
            PRIMARY KEY (`id`, `timestamp`),
            UNIQUE KEY `timestamp` (`timestamp`)
        );''' % table_lookup(device)))

    conn.commit()

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
