import argparse
import paramiko
from scp import SCPClient
from utils import *


def copy_folder(src, dest, connections):
    for connection in connections.items():
        ip,user,pw = extract_conn_infos(connection)
        
        client = create_ssh(ip,user,pw)
        client.exec_command('mkdir -p ' + dest)
        
        scp = SCPClient(client.get_transport())
        scp.put(src, recursive=True, remote_path=dest)
        
        scp.close()
        client.close()
        
def install_monitoring(src, connections):
    copy_folder(src, iot_folder , connections)
    
    for connection in connections.items():
        ip,user,pw = extract_conn_infos(connection)
        client = create_ssh(ip,user,pw)
        
        stdin, _, _ = client.exec_command(f"cd {iot_folder}/monitoring; (sudo -S apt-get -y install ./*)")
        stdin.write(pw + "\n")
        stdin.flush()

        client.close()

def copy_data(src, dest, connections):
    device_no = 1
    for connection in connections.items():
        ip,user,pw = extract_conn_infos(connection)
        
        client = create_ssh(ip,user,pw)
        client.exec_command('mkdir -p ' + dest)
        
        scp = SCPClient(client.get_transport())
        scp.put(f"{src}/part_{device_no}.csv",remote_path=dest)
        
        client.exec_command(f'mv {dest}/part_{device_no}.csv {dest}/data.csv')
        client.exec_command(f'cd {dest}; echo "{device_no}" >> id.txt')
        device_no += 1
        scp.close()
        client.close()
        

            
def main():
    parser = argparse.ArgumentParser(description="Script to deploy data and binaries to the pi's")
    parser.add_argument('-c', '--connections', help='Path to the file with connection information')
    parser.add_argument('-b', '--binaries', help='Path to the binaries of the algorithms')
    parser.add_argument('-d', '--data', help='Path to the folder with the data-sets')
    parser.add_argument('-m', '--monitoring', help='Path to the folder of the monitoring binaries')
    
    args = parser.parse_args()
    
    connections = read_connection_file(args.connections)
    
    if args.data:
        copy_data(args.data, iot_folder + "/data", connections)
        
    if args.binaries:
        copy_folder(args.binaries, iot_folder + "/binaries", connections)
    
    if args.monitoring:
        install_monitoring(args.monitoring, connections)
    
if __name__ == "__main__":
    main()