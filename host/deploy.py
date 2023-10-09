import argparse
import paramiko
from scp import SCPClient
from utils import *

def copy_folder(src, dest, connections):
    for connection in connections.items():
        ip,user,pw = extract_conn_infos(connection)
        
        print(f"Transfer {src} to {dest}")
        
        client = create_ssh(ip,user,pw)
        client.exec_command('mkdir -p ' + dest)
        
        scp = SCPClient(client.get_transport())
        scp.put(src, recursive=True, remote_path=dest)
                
        scp.close()
        client.close()
    
        print(f"Transfer done!")
        

def execute_root_cmd(cmd,pw,client):
    stdin, _, _ = client.exec_command(cmd)
    stdin.write(pw + "\n")
    stdin.flush() 
    
def enable_ntp():
    return

def iptables(src, dest, connections):
    copy_folder(src, dest, connections)
    
    for connection in connections.items():
        ip,user,pw = extract_conn_infos(connection)
        client = create_ssh(ip,user,pw)
        
        print("Installing iptables")        
        execute_root_cmd(f"cd {dest}; (sudo -S DEBIAN_FRONTEND=noninteractive apt-get -yq install ./*)",pw,client)
        print("Installation done!")
        print("Setting up the iptable rules")
        execute_root_cmd("sudo iptables -A INPUT -p tcp -m tcp --dport 22 -j ACCEPT; sudo iptables -A OUTPUT -p tcp --sport 22 -m state --state ESTABLISHED -j ACCEPT", pw, client)
        execute_root_cmd("sudo iptables -A OUTPUT -p tcp --sport 22 -m state --state ESTABLISHED -j ACCEPT", pw,client)
        execute_root_cmd("sudo iptables -A INPUT -p udp --dport 123 -j ACCEPT", pw, client)
        execute_root_cmd("sudo iptables -A OUTPUT -p udp --sport 123 -j ACCEPT", pw, client)
        execute_root_cmd("sudo iptables -A INPUT -p tcp -m tcp --dport 12000:12005 -j ACCEPT", pw, client)
        execute_root_cmd("sudo iptables -A OUTPUT -p tcp --sport 12000:12005 -j ACCEPT", pw, client)
        execute_root_cmd("sudo iptables -A INPUT -p icmp -j ACCEPT", pw, client)
        execute_root_cmd("sudo iptables -P INPUT DROP", pw, client)
        execute_root_cmd("sudo iptables -P OUTPUT DROP", pw, client)
        execute_root_cmd("sudo iptables-save | sudo tee /etc/iptables/rules.v4", pw, client)
        print("Iptables setup done!")
                
        client.close()
        
def install_monitoring(src, dest, connections):
    copy_folder(src, dest, connections)
    
    for connection in connections.items():
        ip,user,pw = extract_conn_infos(connection)
        client = create_ssh(ip,user,pw)
        
        print("Installing monitoring tools!")
        
        stdin, _, _ = client.exec_command(f"cd {dest}; (sudo -S apt-get -y install ./*)")
        stdin.write(pw + "\n")
        stdin.flush()
        
        print("Installation done")

        client.close()

def copy_data(src, dest, connections):
    device_no = 1
    for connection in connections.items():
        ip,user,pw = extract_conn_infos(connection)
        
        print("Connect to: ", ip)
        
        client = create_ssh(ip,user,pw)
        client.exec_command('mkdir -p ' + dest)
        
        print("Start transfer data...")
        
        scp = SCPClient(client.get_transport())
        scp.put(f"{src}/part_{device_no}.csv",remote_path=dest)
        
        client.exec_command(f'mv {dest}/part_{device_no}.csv {dest}/data.csv')
        client.exec_command(f'cd {dest}; echo "{device_no}" >> id.txt')
        
        print("Transfer done!")
        
        device_no += 1
        scp.close()
        client.close()
                    
def main():
    parser = argparse.ArgumentParser(description="Script to deploy data and binaries to the pi's")
    parser.add_argument('-c', '--connections', help='Path to the file with connection information')
    parser.add_argument('-b', '--binaries', help='Path to the binaries of the algorithms')
    parser.add_argument('-d', '--data', help='Path to the folder with the data-sets')
    parser.add_argument('-m', '--monitoring', help='Path to the folder of the monitoring binaries')
    parser.add_argument('-i', '--iptables', help='Path to the folder with the iptable binaries')
    
    args = parser.parse_args()
    
    connections = read_connection_file(args.connections)
    
    if args.data:
        copy_data(args.data, iot_folder + "/data", connections)
        
    if args.binaries:
        copy_folder(args.binaries, iot_folder + "/binaries", connections)
    
    if args.monitoring:
        install_monitoring(args.monitoring, iot_folder + "/monitoring", connections)
    
    if args.iptables:
        iptables(args.iptables, iot_folder + "/iptables", connections)
    
if __name__ == "__main__":
    main()