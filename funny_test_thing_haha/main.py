import GPUtil
import time
import socket
import threading
import queue
import psutil

E = 100000  # Error threshold

CURRENT_K = 0
CURRENT_M = 0

def getValue():
    #return GPUtil.getGPUs()[1].temperature
    return psutil.virtual_memory()._asdict()["used"]

def linear_approximation(nodes):
    N = len(nodes)
    
    # Initialize variables for sum of x, y, xy, and x^2
    sum_x = 0
    sum_y = 0
    sum_xy = 0
    sum_x_squared = 0
    
    # Calculate the sums
    for x_i, y_i in nodes:
        sum_x += x_i
        sum_y += y_i
        sum_xy += x_i * y_i
        sum_x_squared += x_i ** 2
    
    # Calculate the slope (k) of the linear equation (y = kx + m)
    try:
        k = (N * sum_xy - sum_x * sum_y) / (N * sum_x_squared - sum_x ** 2)
    except ZeroDivisionError:
        k = 0

    # Calculate the y-intercept (m) of the linear equation (y = kx + m)
    m = (sum_y - k * sum_x) / N
    
    return (k , m)  # Return the slope

# Initialize a buffer to store the latest 100 samples
sample_buffer = queue.Queue()

# Generate 10 initial samples and store them in the buffer
N = 10000
for _ in range(N):
    sample = getValue()
    t = time.time()
    sample_buffer.put((t, sample))

# Define the target host and port
host = 'localhost'
core = 12000
port = 12001

def thread_ref():
    while True:
        ref_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        try:
            ref_socket.connect((host, core))
            while True:
                temp = getValue()
                ref_socket.send(f"1,{t:.6f},{temp}\n".encode('utf-8'))
                
                time.sleep(1)
        except:
            print("Restarting ref socket...")
            ref_socket.close()
            time.sleep(1)

def non_thread_ref():
    ref_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        ref_socket.connect((host, core))
        temp = getValue()
        ref_socket.send(f"1,{t:.6f},{temp}\n".encode('utf-8'))
    except:
        print("Ref error")
        ref_socket.close()


#ref_thread = threading.Thread(target=thread_ref)
#ref_thread.start()



while True:
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        client_socket.connect((host, port))

        while True:
            time.sleep(1/100)
            # Remove the oldest sample
            sample_buffer.get()

            # Add a new sample
            sample = getValue()
            t = time.time()
            sample_buffer.put((t, sample))

            non_thread_ref()

            # Calculate and print the linear approximation
            funcKM = linear_approximation(list(sample_buffer.queue))
            #print(f"Linear Approximation: y = {funcKM[0]} * t + {funcKM[1]}")
            #print(funcKM[0] * t + funcKM[1], " actual: ", getValue())

            # check if error is greater than E
            print((funcKM[0] * t + funcKM[1]) - (CURRENT_K * t + CURRENT_M), end="")
            if abs((funcKM[0] * t + funcKM[1]) - (CURRENT_K * t + CURRENT_M)) > E:
                print(f" error greater than E: {E}", end="")
                CURRENT_K = funcKM[0]
                CURRENT_M = funcKM[1]
                client_socket.send(f"0,{t:.6f},{funcKM[0]:.15f},{funcKM[1]:.15f}".encode("utf-8"))
            print()
    except:
        print("Restarting socket...")
        client_socket.close()
        time.sleep(1)