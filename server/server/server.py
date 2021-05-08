import socket
import sys
import os
import random
import subprocess
from _thread import *
from threading import Lock
from scrappy_network import *

delimiter = "<::>"
max_conns = 30
host = socket.gethostname()
  
ip_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
ip_socket.connect(("192.168.88.221", 47556))# 10.42.0.136 # Adjust IP to fit a client 
serverIP = ip_socket.getsockname()[0]
ip_socket.close()

os.system("ping -c 1 " + "192.168.88.221") # Adjust IP to fit a client

print("This is: " + host) 
print("This IP Address is: " + serverIP)
port = 47556 # ports we use: 46337-46998, 47101-47557, 47555 for http server
skt = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
try:
    skt.bind(('', port))
except socket.error as e:
    print(str(e))
    
ports_in_use = [47556]
port_mutex = Lock()

path_to_matryoshka = "../matryoshka/Release/matryoshkaModular"
path_to_library = "../scrappyLibrary/" # 
# TODO: Split scrap library and model locations so that scrap can be named 1, 2, 3 ...
path_to_models = path_to_library + "exportedDesigns/"

def execute_command(shell_command):
    p = subprocess.Popen(shell_command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    cmd_output = ""
    
    return_value = p.wait()

    for line in p.stdout.readlines():
        cmd_output += str(line)[2:len(str(line))-3] + " \n"

    
    # print(return_value)
    # streamdata = p.communicate()[0]
    # rc = p.returncode
    # print(rc)
    return [return_value, cmd_output]

# extract data array from matryoshka output while avoiding writing another file
def parse_matryoshka_info(info_string):
    try:
        info_string_parts = info_string.split("<!..>")
        info_string = info_string_parts[1] + info_string_parts[3] + info_string_parts[5] # 0=before, 1=preload-Translations, 2=moreOutputStuff, 3=data, 4=after, 5=MatrixData, 6=after #TODO: Might fail without Matryoshka_Debug flag
    except:
        return []
    # default vector formatting might put many spaces in
    info_string = info_string.replace("    ", " ").replace("   ", " ").replace("  ", " ").replace("> ", ">").replace(" <", "<")
    info_array = info_string.split("<..>")
    return info_array

# selecting unused port
def acquire_port():
    port_mutex.acquire()
    try:
        file_port = random.randrange(47101,47555,1)
        while(file_port in ports_in_use):
            file_port = random.randrange(47101,47555,1)
        ports_in_use.append(file_port)
        print("New port acquired, ports in use: " + str(ports_in_use))
    finally:
        port_mutex.release()
        return file_port

# release port when no longer in use
def release_port(port):
    port_mutex.acquire()
    try:
        if(port in ports_in_use):
            ports_in_use.remove(port)
        print("Port released, ports in use: " + str(ports_in_use))
    finally:
        port_mutex.release()

# thread definition, handles all logic
skt.listen(max_conns)
print('Waiting for a connection.')
def threaded_server(conn, address):
    send_msg(conn, str.encode("ACC"))
    #file_base_path = "/home/hci-haptics-lab/Desktop/IN/"
    #file_base_path_send = "/home/hci-haptics-lab/Desktop/"
    file_base_path = path_to_library
    file_base_path_send = path_to_models
    
    client_host = address[0]
    client_port = str(address[1])

    while True:
        data = recv_msg(conn)

        # do nothing without receiving a message
        if not data:
            break

        # decode and split message contents
        data_array = data.decode('utf-8').split(delimiter)
        print("Message data: " + str(data_array))

        # RESPONSE HANDLING
        # handle receiving a file sent by the client 
        if (data_array[0] == "SEND_FILE"):
            file_name = data_array[1]
            file_mtime = data_array[2]
            file_size = data_array[3]
            force = False
            if (data_array[4] == "FORCE"):
                force = True
            path_to_file = file_base_path + file_name

            # check if a local copy of the file already exists
            if (os.path.isfile(path_to_file)):
                check_size = str(os.stat(path_to_file).st_size) # local file size
                check_time = str(os.stat(path_to_file).st_mtime) # local file modification date
            else:
                check_size = 0
                check_time = 0

            # check if local copy has same size (and is newer)
            if (os.path.isfile(path_to_file) and check_size == file_size and check_time >= file_mtime and not force):
                print("No need to receive file, existing copy newer than transfer file, same size")
                reply = "FILE_ALREADY_EXISTS"
                send_msg(conn, str.encode(reply))
            # no valid local file, receive file 
            else:
                file_transfer_port = acquire_port()
                print("File transfer new port: " + str(file_transfer_port)) 
                file_transfer_port_string = str(file_transfer_port)
                # MSGCODE, Port
                reply = "WAITING_FOR_FILE" + delimiter + file_transfer_port_string
                send_msg(conn, str.encode(reply))
                network_receive_file(file_base_path, file_name, file_size, host, file_transfer_port)
                release_port(file_transfer_port)

        # handle client requesting to send a file to them
        elif (data_array[0] == "REQUEST_FILE"):
            file_name = data_array[1]
            file_mtime = data_array[2]
            file_size = data_array[3]
            force = False
            if (data_array[4] == "FORCE"):
                force = True
            path_to_file = file_base_path_send + file_name
            if (os.path.isfile(path_to_file)):
                check_size = str(os.stat(path_to_file).st_size) # local file size
                check_time = str(os.stat(path_to_file).st_mtime) # local file modification date

                if (check_size == file_size and check_time <= file_mtime and not force):
                    print("No need to send file, requested file older than existing copy, same size")
                    reply = "NO_UPDATED_FILE_AVAILABLE"
                    send_msg(conn, str.encode(reply))
                else:
                    # file_transfer_port = acquire_port()
                    # print("File transfer new port: " + str(file_transfer_port))
                    # file_transfer_port_string = str(file_transfer_port)
                    # MSGCODE, Port, File_mTime, File_size
                    # reply = "START_WAITING_FOR_REQUESTED_FILE" + delimiter + file_transfer_port_string + delimiter + check_time + delimiter + check_size
                    reply = "START_WAITING_FOR_REQUESTED_FILE" + delimiter + check_time + delimiter + check_size
                    send_msg(conn, str.encode(reply))
            else:
                print("Requested file does not exist")
                reply = "FILE_DOES_NOT_EXIST"
                send_msg(conn, str.encode(reply))

        # handle sending a file to client after the handshake was successful
        elif (data_array[0] == "READY_TO_RECEIVE_FILE"):
            file_name = data_array[1]
            file_transfer_port = int(data_array[2])
            path_to_file = file_base_path_send + file_name
            print("Sending file: " + file_name + " to: " + client_host + ":" + data_array[2])
            network_send_file(file_base_path_send, file_name, client_host, file_transfer_port)
            print("File sent")
            # release_port(file_transfer_port)
        
        # handle the request to perform an arbitrary command (TODO: FIX SECURITY ISSUES)
        elif (data_array[0] == "EXECUTE_COMMAND"):
            path_to_executable = data_array[1]
            options_string = data_array[2]
            path_to_outer_file = data_array[3]
            path_to_inner_file = data_array[4]
            print("Executing command: " + path_to_executable)
            
            shell_command = path_to_executable + " " + options_string + " " + path_to_outer_file + " " + path_to_inner_file
            command_result = execute_command(shell_command)
            # MSGCODE, Port
            reply = "COMMAND_RESULT" + delimiter + str(command_result[0]) + delimiter + command_result[1]
            send_msg(conn, str.encode(reply))

        # handle request to run matryoshka
        elif (data_array[0] == "EXECUTE_MATRYOSHKA"):
            options_string = data_array[1]
            path_to_outer_file = file_base_path + data_array[2]
            path_to_inner_file = file_base_path + data_array[3]

            hq_string = ""

            if (data_array[4] != ""):
                hq_string += "-c '" + file_base_path + data_array[4] + "'"
            if (data_array[5] != ""):
                if (data_array[4] != ""):
                    hq_string += " "
                hq_string += "-d '" + file_base_path + data_array[5] + "'"    

            print("Executing Matryoshka")

            shell_command = path_to_matryoshka + " " + options_string + " " + hq_string + " " + path_to_outer_file + " " + path_to_inner_file
            shell_command = shell_command.replace("  ", " ").replace("  "," ").replace("  "," ")
            command_result = execute_command(shell_command)
            
            print(shell_command)

            if (str(command_result[0]) == "0"):
                info_array = parse_matryoshka_info(command_result[1])
                print(str(len(info_array)) + str(info_array))
                print(str(info_array))
                # MSGCODE, exit_code, std_out, position_info
                reply = "MATRYOSHKA_SUCCESS" + delimiter + str(command_result[0]) + delimiter + command_result[1] + delimiter + str(info_array)
                send_msg(conn, str.encode(reply))
            elif(str(command_result[0]) == "1"):
                # MSGCODE, exit_code, std_out
                reply = "MATRYOSHKA_UNSUCCESSFUL" + delimiter + str(command_result[0]) + delimiter + command_result[1]
                send_msg(conn, str.encode(reply))
            else:
                info_array = parse_matryoshka_info(command_result[1])
                if (len(info_array) == 8):
                    # MSGCODE, exit_code, std_out, position_info
                    reply = "MATRYOSHKA_ERROR_STL" + delimiter + str(command_result[0]) + delimiter + command_result[1] + delimiter + str(info_array)
                    send_msg(conn, str.encode(reply))
                else:
                    # MSGCODE, exit_code, std_out
                    reply = "MATRYOSHKA_ERROR_UNKOWN" + delimiter + str(command_result[0]) + delimiter + command_result[1]
                    send_msg(conn, str.encode(reply))
        print("")

    print("No more data received, closing connection")
    print("_________________________________________\n")
    conn.close()

# "main"
while True:
    conn, addr = skt.accept()
    print('connected to: '+addr[0]+':'+str(addr[1]) + ", starting new thread")

    start_new_thread(threaded_server,(conn, addr))




