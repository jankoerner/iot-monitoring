import argparse
import os
import paramiko
import time
from scp import SCPClient
from utils import *

def startup(connections):
    for connection in connections.items():
        ip, user, pw = extract_conn_infos(connection)
        
        client = create_ssh(ip,user,pw)      
        client.exec_command(f"cd {iot_folder}/; (nohup ./startup.sh &); cd ~")
        client.close()

def start_monitoring(connections, interval, total_ticks):
    for connection in connections.items():
        ip, user, pw = extract_conn_infos(connection)
        
        filename = str(round(time.time() * 1000)) + ".stats"
        
        client = create_ssh(ip,user,pw)      
        client.exec_command(f"mkdir -p {iot_folder}/monitoring/results")
        
        cmd = f"cd {iot_folder}/monitoring/results;(sar -u -n DEV -r -d -o {filename} {interval} {total_ticks} > /dev/null 2>&1 &)"        
        client.exec_command(cmd)
        client.close()
    

def main():
    parser = argparse.ArgumentParser(description="Script to startup the algorithms and the monitoring")
    parser.add_argument('-c', '--connections', help='Path to the file with connection information')
    parser.add_argument('-m', '--monitoring', help='If the monitoring should be started', action="store_true")
    parser.add_argument('-i', '--interval', help='The interval in which stats are monitored in seconds (default: %(default)s)', default=1)
    parser.add_argument('-d', '--duration', help='The duration in which the stats should be monitored in minutes (default: %(default)s)', default=30)
    parser.add_argument('-a', '--algorithm', 
                        help='Which algorithm should be started (default: %(default)s)', 
                        default='baseline',
                        const='default',
                        nargs='?',
                        choices=['baseline'])
    
    args = parser.parse_args()
    
    connections = read_connection_file(args.connections)
    
    if args.monitoring:
        ticks_per_minute = 60 // float(args.interval) 
        total_ticks = ticks_per_minute * float(args.duration)       
        start_monitoring(connections, args.interval, int(total_ticks))  
        

if __name__ == "__main__":
    main()