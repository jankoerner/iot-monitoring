import argparse
import csv
import os
import paramiko
from scp import SCPClient

iot_folder = '~/iot-monitoring/'
            
def create_ssh(ip, user, pw):
    ssh_client = paramiko.SSHClient()
    ssh_client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    ssh_client.connect(ip, username=user, password=pw) 
    
    return ssh_client

def startup(connections):
    for connection in connections.items():
        ip = connection[0]
        user = connection[1][0]
        pw = connection[1][1]
        
        client = create_ssh(ip,user,pw)      
        client.exec_command(f"cd {iot_folder}/; (nohup ./startup.sh &); cd ~")
        client.close()

def copy_folder(src, dest, connections):
    for connection in connections.items():
        ip = connection[0]
        user = connection[1][0]
        pw = connection[1][1]
        
        client = create_ssh(ip,user,pw)
        client.exec_command('mkdir -p ' + dest)
        
        scp = SCPClient(client.get_transport())
        scp.put(src, recursive=True, remote_path=dest)
        
        scp.close()
        client.close()

def main():
    parser = argparse.ArgumentParser(description="Script to deploy data and binaries to the pi's")
    parser.add_argument('-c', '--connections', help='Path to the file with connection information')
    parser.add_argument('-d', '--data', help='Path to folder with the data-sets')
    parser.add_argument('-b', '--binaries', help='Path to the binaries of the algorithms')
    parser.add_argument('-s', '--startup', help='If the startup script should run', action='store_true')
    
    args = parser.parse_args()
    
    connections = {}
    with open(args.connections, 'r') as file:
        reader = csv.DictReader(file)
        for row in reader:
            ip = row['ip']
            user = row['user']
            pw = row['password']            
            connections[ip] = (user, pw)
    
    if args.data:
        copy_folder(args.data, iot_folder + "data", connections)
        
    if args.binaries:
        copy_folder(args.binaries, iot_folder + "binaries", connections)
    
    if args.startup:
        startup(connections)
        

if __name__ == "__main__":
    main()