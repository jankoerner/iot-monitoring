#!/usr/bin/python

import socket

import sip_sink as sip

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

table_lookup = {
    1: "table_a",
    2: "table_b",
    3: "table_c",
    4: "table_d",
    5: "table_e",
    6: "table_f"
}

cursor = conn.cursor()

insert = '''INSERT INTO `%s` (`timestamp`, `value`) VALUES ('%s', %s);'''

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
                table = table_lookup[int(type_s)]
            except:
                print("Invalid not a type, continue")
                continue

            #unixtime = "{:.6f}".format(float(time_s))
            timestamp = datetime.datetime.fromtimestamp(unixtime).strftime('%Y-%m-%d %H:%M:%S.%f')
            val = float(value_s)
        
            try:
                stmnt = (insert % (table, timestamp, val))
                cursor.execute(stmnt)
                conn.commit()
            except:
                print("Could not insert into database, connection close")
                break
            
            print(f"Inserted: {val} into {table} @ {timestamp}")        
            
    # connection closed
    c.close()


def Main():

    for alg in table_lookup:
        # create table if none exists
        cursor.execute(('''CREATE TABLE IF NOT EXISTS `%s` (
            `id` int NOT NULL AUTO_INCREMENT,
            `timestamp` DATETIME(6) NOT NULL,
            `value` double NOT NULL,
            PRIMARY KEY (`id`, `timestamp`),
            UNIQUE KEY `timestamp` (`timestamp`)
        );''' % table_lookup[alg]))

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