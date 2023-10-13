from enum import Enum
import socket
import math
import socket
import os
import threading
import time

NUM_DEVICES = int(os.environ['NUM_DEVICES'])
HOST = "0.0.0.0"  # Standard loopback interface address (localhost)
PORT = int(os.environ['LMS_PORT'])  # Port to listen on
CORE_PORT = int(os.environ['CORE_PORT'])  # Core port to connect to

class LMSFilter:
    def __init__(self, state, deviceId):
        self.state = state #0 = INITIALIZATION, 1 = NORMAL, 2 = STANDALONE
        self.deviceId = deviceId
        self.window = []
        self.weights = []
        self.learningRate = 0.0
        self.windowSize = 0
        self.sampleRate = 0
        self.predictionThread = None
        self.stopEvent = threading.Event()
        self.coreSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.coreSocket.connect(("core", CORE_PORT))

    def initialize(self, windowSize, sampleRate):
        self.windowSize = windowSize
        self.sampleRate = sampleRate
        for _ in range(0, self.windowSize):
            self.weights.append(0.0)
            self.window.append(0.0)
            print(f"LMS with DeviceId: {self.deviceId} initialized!")

    def send_msg(self, value):
        print("Value sent:", value)
        t = int(time.time() * 1_000_000)
        msg = f"{self.deviceId},4,{t},{value:.12f}\n".encode('utf-8')
        self.coreSocket.sendall(msg)

    def predict(self):
        prediction = sum(w * x for w, x in zip(self.weights, self.window))
        return prediction        
    
    def update_weights(self,prediction,target):
        error = target - prediction
        for i in range(self.windowSize):
            self.weights[i] += self.learningRate * error * self.window[i]

    def update_window(self, value):
        self.window.pop(0)
        self.window.append(value)

    def calc_learning_rate(self):
        tempSum = sum(math.pow(abs(v), 2) for v in self.window)
        ex = (1.0 / self.windowSize) * tempSum
        upperBound = 1.0 / ex
        self.learningRate = upperBound / 100.0
        print("LearningRate:", self.learningRate)

    def normal(self, value):
        prediction = self.predict()
        self.update_weights(prediction,value)
        self.update_window(value)
        self.send_msg(value)
    
    def standalone(self):
        while True:
            time.sleep(self.sampleRate/1000)
            if(self.stopEvent.is_set()):
                self.stopEvent.clear()
                return
            prediction = self.predict()
            self.update_window(prediction)
            self.send_msg(prediction)

    def process_value(self, value, nextState):
        if nextState == 0:
            self.update_window(value)
            self.send_msg(value)
        elif nextState == 1:
            if self.state == 0:
                self.calc_learning_rate()
                self.update_window(value)
            elif self.state == 1:
                self.normal(value)
            elif self.state == 2:
                self.normal(value)
                self.stop_thread()
        elif nextState == 2:
            if self.state == 1:
                self.normal(value)
                self.predictionThread = threading.Thread(target=self.standalone)
                self.predictionThread.start()
                    
        self.state = nextState
    
    def stop_thread(self):
        self.stopEvent.set()
        self.predictionThread.join()

    def process_data(self,data):
        id = int(data[0])
        if id != self.deviceId:
            print(f"Device Id mismatch! Expected: {self.deviceId} but received: {id}")
        # data has following elements: [id,msgId,....]
        # msgId = "INIT": [id,"INIT",windowSize,sampleRate]
        # msgId = "DATA": [id,"DATA",value,nextState]
        
        if data[1] == "INIT":
            self.initialize(int(data[2]),int(data[3]))
        elif data[1] == "DATA":
            self.process_value(float(data[2]),int(data[3]))
        elif data[1] == "STOP":
            self.stop_thread()

filters = [None] * NUM_DEVICES
for i in range(0,NUM_DEVICES):
    filters[i] = LMSFilter(0, i+1)


def handle_data(rawData):
    print("Raw Data:", rawData)
    data = rawData.split(",")
    deviceId = int(data[0]) - 1
    filters[deviceId].process_data(data)


s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.bind((HOST, PORT))

print(f"Started server on: {HOST}:{PORT}")

s.listen()

while True:
    client_socket, client_address = s.accept()
    with client_socket:
        while True:
            data = client_socket.recv(1024)  # Adjust the buffer size as needed
            if not data:
                break
            handle_data(data.decode())  # Assuming data is in UTF-8 encoding

    
