# echo-server.py

import socket

HOST = "localhost"  # Standard loopback interface address (localhost)
PORT = 12000  # Port to listen on (non-privileged ports are > 1023)

print(f"Starting server on: {HOST}:{PORT}")


with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.bind((HOST, PORT))
    s.listen()
    conn, addr = s.accept()
    with conn:
        print(f"Connected by {addr}")
        # get data untill newline and print
        data = conn.recv(1024)
