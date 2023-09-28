 from _thread import *
import threading
import time

class SIP:
    def __init__(self, k, m):
        self.k = k
        self.m = m
        self.lastSampledTime = 0
    
    def getPrediction(time):
        self.lastSampledTime = float(f"{time.time():.6f}")
        return self.k * time + self.m
    
    def getFunction(time):
        return [self.k, self.m]

p1 = SIP(1,2)


# Setup socket

def init(HOST, PORT):
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.bind((HOST, PORT))

    print(f"Started server on: {HOST}:{PORT}")

    # Listen for incoming connections
    s.listen(1)  # Listen for a single incoming connection

    c, addr = s.accept()
    
    start_new_thread(threaded, (c,))

    
    return sink

# Start service

# Spawn thread that samples the sink
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

            if(len(args) != 4):
                print("Invalid data format, continue")
                continue
 
            #(INT, TIME, FLOAT)

            # Check types
            type_s  = args[0]
            time_s  = args[1]
            k_s = args[2]
            m_s = args[2]

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
            if (not k_s.replace('.', '', 1).isdigit()): 
                print("Invalid data format, continue")
                
            # check if arg3 is a float
            if (not m_s.replace('.', '', 1).isdigit()): 
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
            k = float(k_s)
            m = float(m_s)
         
            return [k, m]
            
    # connection closed
    c.close()
