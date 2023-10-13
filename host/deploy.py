import argparse
import paramiko
import os
from scp import SCPClient
from utils import *

def copy_folder(src, dest, connections):
    for connection in connections.items():
        ip,user,pw = extract_conn_infos(connection)
        
        print(f"Transfer {src} to {ip}:{dest}")
        
        client = create_ssh(ip,user,pw)
        client.exec_command('mkdir -p ' + dest)
        
        scp = SCPClient(client.get_transport())
        scp.put(src, recursive=True, remote_path=iot_folder)
                
        scp.close()
        client.close()
    
        print(f"Transfer done!")

def retrieve_results(connections, dest):
    for connection in connections.items():
        ip,user,pw = extract_conn_infos(connection)
        client = create_ssh(ip,user,pw)
        sftp = client.open_sftp()

        dataFiles = sftp.listdir("/home/pi/iot-monitoring/monitoring/results") 

        for file in dataFiles:
            if file.split(".")[1] == "stats":
                print(f"Create csv file for {file} on {ip}")
                cmd = (f"cd /home/pi/iot-monitoring/monitoring/results;sadf -dh -- -u -n DEV -r -d -F --iface=eth0 --dev=mmcblk0 {file} > {file}.csv")
                client.exec_command(cmd)
                remoteFile = f"/home/pi/iot-monitoring/monitoring/results/{file}.csv"
                localPath = os.getcwd()
                print(f"Copy file to {localPath}")
                sftp.get(remoteFile, f"{localPath}/results/{file}.csv")
        
        sftp.close()
        client.close()


def execute_root_cmd(cmd,pw,client):
    stdin, stdout, _ = client.exec_command(cmd)
    stdin.write(pw + "\n")
    stdin.flush() 
    stdout.channel.recv_exit_status()

def iptables(src, dest, connections, persistent):
    copy_folder(src, dest, connections)
    
    for connection in connections.items():
        ip,user,pw = extract_conn_infos(connection)
        client = create_ssh(ip,user,pw)
        
        print(f"Installing iptables on: {ip}")        
        execute_root_cmd(f"cd {dest}; (sudo -S DEBIAN_FRONTEND=noninteractive apt-get -yq install ./*)",pw,client)
        print("Installation done!")
        print(f"Setting up the iptable rules on: {ip}")
        #execute_root_cmd("sudo sudo iptables -F",pw,client) # This cmd sometimes takes a lot of time, I have no Idea why
        execute_root_cmd("sudo iptables -A INPUT -p tcp -m tcp --dport 22 -j ACCEPT; sudo iptables -A OUTPUT -p tcp --sport 22 -m state --state ESTABLISHED -j ACCEPT", pw, client)
        execute_root_cmd("sudo iptables -A INPUT -p udp --sport 123 -j ACCEPT; sudo iptables -A OUTPUT -p udp --dport 123 -j ACCEPT", pw, client)
        execute_root_cmd("sudo iptables -A INPUT -p tcp -m tcp --sport 12000:12005 -j ACCEPT; sudo iptables -A OUTPUT -p tcp --dport 12000:12005 -j ACCEPT", pw, client)
        execute_root_cmd("sudo iptables -A INPUT -p icmp -j ACCEPT", pw, client)
        execute_root_cmd("sudo iptables -P OUTPUT DROP; sudo iptables -P INPUT DROP", pw, client)
        if persistent:
            execute_root_cmd("sudo iptables-save | sudo tee /etc/iptables/rules.v4 >> /dev/null", pw, client)
        print("Iptables setup done!")
                
        client.close()
        
def install_monitoring(src, dest, connections):
    copy_folder(src, dest, connections)
    
    for connection in connections.items():
        ip,user,pw = extract_conn_infos(connection)
        client = create_ssh(ip,user,pw)
        
        print(f"Installing monitoring tools on {ip}")
        
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
        
        print(f"Transfer {src}/part_{device_no}.csv to {ip}:{dest}")
        
        scp = SCPClient(client.get_transport())
        scp.put(f"{src}/part_{device_no}.csv",remote_path=dest)
        
        client.exec_command(f'mv {dest}/part_{device_no}.csv {dest}/data.csv')
        client.exec_command(f'cd {dest}; echo "{device_no}" > id.txt')
        
        print("Transfer done!")
        
        device_no += 1
        scp.close()
        client.close()

def start_ntp(server_ip,connections):
    for connection in connections.items():
        ip,user,pw = extract_conn_infos(connection)
        print("Enable ntp on:", ip)

        client = create_ssh(ip,user,pw)
        execute_root_cmd("sudo timedatectl set-ntp true", pw, client)
        execute_root_cmd(f'echo -e "[Time]\nNTP={server_ip}\nPollIntervalMinSec=16\nPollIntervalMaxSec=2048" | sudo tee /etc/systemd/timesyncd.conf >> /dev/null', pw, client)
        execute_root_cmd("sudo systemctl enable systemd-timesyncd.service", pw, client)
        execute_root_cmd("sudo systemctl restart systemd-timesyncd.service", pw, client)

        client.close()
        print("Ntp enabled!")

                    
def main():
    parser = argparse.ArgumentParser(description="Script to deploy data and binaries to the pi's")
    parser.add_argument('-c', '--connections', help='Path to the file with connection information')
    parser.add_argument('-b', '--binaries', help='Path to the binaries of the algorithms')
    parser.add_argument('-d', '--data', help='Path to the folder with the data-sets')
    parser.add_argument('-m', '--monitoring', help='Path to the folder of the monitoring binaries')
    parser.add_argument('-i', '--iptables', help='Path to the folder with the iptable binaries')
    parser.add_argument('-n', '--ntp', help="If the ntp-service should be started", action="store_true")
    parser.add_argument('-r', '--results', help="Creates a csv file from the result, then transfers them to the host", action="store_true")
    
    args = parser.parse_args()
    
    connections = read_connection_file(args.connections)
    
    if args.data:
        copy_data(args.data, iot_folder + "/data", connections)
        
    if args.binaries:
        copy_folder(args.binaries, iot_folder + "/binaries", connections)
    
    if args.monitoring:
        install_monitoring(args.monitoring, iot_folder + "/monitoring", connections)
    
    if args.iptables:
        iptables(args.iptables, iot_folder + "/iptables", connections, False)

    if args.ntp:
        start_ntp("200.150.100.10",connections)
    
    if args.results:
        retrieve_results(connections,"results")
    
if __name__ == "__main__":
    main()