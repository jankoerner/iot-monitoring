import argparse
import os
import paramiko
import time
from scp import SCPClient
from utils import *
import concurrent.futures

SERVER_IP = "200.150.100.10"
CORE_PORT = 12000
SIP_PORT = 12001
LMS_PORT = 12004

DATA_FILE_PATH = iot_folder + "/data/data.csv"
ID_FILE_PATH = iot_folder + "/data/id.txt"

def algorithm_port(algo):
    match algo:
        case "lms":
            return LMS_PORT
        case "sip" | "sip-ewma":
            return SIP_PORT
        case _:
            return CORE_PORT

def algorithm_id(algo):
    match algo:
        case "baseline":
            return 1
        case "static":
            return 2
        case "static-mean":
            return 3
        case "lms":
            return 4
        case "sampling":
            return 5
        case "pla":
            return 6
        case "sip-ewma":
            return 7
        case "sip":
            return 8
        case _:
            return 1

def cpp_filter_cmd(algo, interval, duration, pretty):
    algoId = algorithm_id(algo)
    algoPort = algorithm_port(algo)
    cmd = f"cd {iot_folder}/binaries/; (nohup ./cppfilter {DATA_FILE_PATH} {interval} {duration} {SERVER_IP} {algoPort} {ID_FILE_PATH} {algoId} {pretty} > test.txt 2>&1 <&- &)"
    return cmd

def startup(connection, cmd):
    ip, user, pw = extract_conn_infos(connection)
    print(f"Start algorithm on: {ip}")
    client = create_ssh(ip,user,pw)      
    client.exec_command(cmd)
    client.close()

def start_monitoring(connection, interval, total_ticks, algo):
    ip, user, pw = extract_conn_infos(connection)

    print(f"Start monitoring on: {ip}")             
    
    client = create_ssh(ip,user,pw)      
    sftp = client.open_sftp()
    device_id = sftp.file("/home/pi/iot-monitoring/data/id.txt",'r').readline().rstrip()
    sftp.close()
    filename = f"{device_id}_{algo}.stats"

    client.exec_command(f"mkdir -p {iot_folder}/monitoring/results")
    cmd = f"cd {iot_folder}/monitoring/results;(sar -u -n DEV -r -d -F -o {filename} {interval} {total_ticks} > /dev/null 2>&1 &)"
    
    client.exec_command(cmd)
    client.close()


def execute(connection,args):
    try:
        if args.monitoring:
            interval = int(float(args.interval)) if float(args.interval) > 1 else 1
            ticks_per_minute = 60 // float(interval) 
            total_ticks = ticks_per_minute * float(args.duration)       
            start_monitoring(connection, interval, int(total_ticks), args.algorithm)  
            time.sleep(5)

        cmd = ""
        match args.algorithm:
            case "baseline" | "static" | "static-mean" | "lms":
                cmd = cpp_filter_cmd(args.algorithm, args.freq, args.runtime, 1 if args.pretty else 0)
            case "sampling":
                secs = int(args.runtime) * 60
                cmd = f"cd {iot_folder}/binaries/; (nohup ./main -A {SERVER_IP} -d {secs} -t {args.freq} > /dev/null 2>&1 &)"
            case "pla":
                secs = int(args.runtime) * 60
                cmd = f"cd {iot_folder}/binaries/; (nohup ./pla -A {SERVER_IP} -d {secs} -t {args.freq} > /dev/null 2>&1 &)"
            case "sip" :
                frequency = 1000 // args.freq
                secs = int(args.runtime) * 60
                cmd = f"cd {iot_folder}/binaries/; (nohup ./sip {DATA_FILE_PATH} {SERVER_IP} {SIP_PORT} {frequency} {secs} 20 2 1 > /dev/null 2>&1 &)"
            case "sip-ewma":
                frequency = 1000 // args.freq
                secs = int(args.runtime) * 60
                cmd = f"cd {iot_folder}/binaries/; (nohup ./sip {DATA_FILE_PATH} {SERVER_IP} {SIP_PORT} {frequency} {secs} 20 2 0.25 > /dev/null 2>&1 &)"
                print(cmd)
            case _ :
                cmd = cpp_filter_cmd("baseline",args.freq, args.runtime)
        
        startup(connection, cmd)
    except Exception as e:
        raise Exception(f"Startup on {connection[0]} failed with {e}!")


def main():
    parser = argparse.ArgumentParser(description="Script to startup the algorithms and the monitoring")
    parser.add_argument('-c', '--connections', help='Path to the file with connection information')
    parser.add_argument('-m', '--monitoring', help='If the monitoring should be started', action="store_true")
    parser.add_argument('-i', '--interval', help='The interval in which stats are monitored in seconds (default: %(default)s)', default=1)
    parser.add_argument('-d', '--duration', help='The duration in which the stats should be monitored in minutes (default: %(default)s)', default=1)
    parser.add_argument('-a', '--algorithm', 
                        help='Which algorithm should be started (default: %(default)s)', 
                        default='baseline',
                        const='default',
                        nargs='?',
                        choices=['baseline', 'static', 'static-mean', 'lms', 'sampling', 'pla', 'sip-ewma', 'sip'])
    parser.add_argument('-f', '--freq', help='Sample freq of the algorithm in ms (default: %(default)s)', default=1000)
    parser.add_argument('-r', '--runtime', help='The runtime of the algorithm in minutes (default: %(default)s)', default=5)
    parser.add_argument('-p', '--pretty', help="If pretty graphs are necessary", action="store_true")


    args = parser.parse_args()
    connections = read_connection_file(args.connections)

    executor = concurrent.futures.ThreadPoolExecutor(max_workers=os.cpu_count()) 
    futures = [executor.submit(execute,connection,args) for connection in connections.items()]

    done, not_done = concurrent.futures.wait(futures, return_when=concurrent.futures.ALL_COMPLETED)

    for future in done:
        if future.exception():
            print(future.exception())
   
    executor.shutdown()

if __name__ == "__main__":
    main()