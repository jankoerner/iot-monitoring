import csv
import paramiko

iot_folder = '~/iot-monitoring'

def create_ssh(ip, user, pw):
    ssh_client = paramiko.SSHClient()
    ssh_client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    ssh_client.connect(ip, username=user, password=pw, timeout=5) 
    
    return ssh_client

def extract_conn_infos(connection):
    ip = connection[0]
    user = connection[1][0]
    pw = connection[1][1]
    
    return ip,user,pw

def read_connection_file(src):
    connections = {}
    with open(src, 'r') as file:
        reader = csv.DictReader(file)
        for row in reader:
            ip = row['ip']
            user = row['user']
            pw = row['password']            
            connections[ip] = (user, pw)
    
    return connections