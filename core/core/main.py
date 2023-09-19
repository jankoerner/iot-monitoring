#!/usr/bin/python

import socket

HOST = "0.0.0.0"  # Standard loopback interface address (localhost)
PORT = 12000  # Port to listen on (non-privileged ports are > 1023)

# import thread module
from _thread import *
import threading
import time
import datetime

import mysql.connector

conn = mysql.connector.connect(
    host="mysql",
    user="user",
    password="password",
    database="core_db"
)

cursor = conn.cursor()

insert = '''INSERT INTO `values` (`timestamp`, `value`) VALUES ("%s", %s);'''

# thread function
def threaded(c):
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
                val = float(line)
            except ValueError:
                print("Not a float, continue")
                continue
            try:
                now = datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S.%f')
                print(insert % (now, val))
                cursor.execute((insert % (now, val)))
                conn.commit()
            except:
                print("Could not insert into database, connection close")
                break
            
            print(f"Inserted: {val}")

        
            
    # connection closed
    c.close()


def Main():
    
    # create table if none exists
    cursor.execute('''CREATE TABLE IF NOT EXISTS `values` (
        `id` int NOT NULL AUTO_INCREMENT,
        `timestamp` DATETIME(6) NOT NULL,
        `value` double DEFAULT NULL,
        PRIMARY KEY (`id`)
    );''')

    conn.commit()

    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.bind((HOST, PORT))

    print(f"Started server on: {HOST}:{PORT}")

    # put the socket into listening mode
    s.listen(5)
    print("socket is listening")

    # a forever loop until client wants to exit
    while True:

        # establish connection with client
        c, addr = s.accept()

        print('Connected to :', addr[0], ':', addr[1])

        # Start a new thread and return its identifier
        start_new_thread(threaded, (c,))
    s.close()


if __name__ == '__main__':
    Main()