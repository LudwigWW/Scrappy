import socket
import sys
import os
import time
import random
from scrappy_network import *
import subprocess # TODO: remove
import asyncio
from threading import Lock


delimiter = "<::>"
local_host = ""
target_host =  "localhost"
target_port = 47556

ports_in_use = [47556]
port_mutex = Lock()

# selecting unused port
def acquire_port():
    port_mutex.acquire()
    try:
        file_port = random.randrange(46337,46997,1)
        while(file_port in ports_in_use):
            file_port = random.randrange(46337,46997,1)
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


def send_file(socket, file_base_path, file_name, host, port, force):
    # while (True):
    if(not os.path.isfile(file_base_path + file_name)):
        raise Exception("File to send does not exist at path: " + file_base_path + file_name) 
    file_stats = os.stat(file_base_path + file_name)
    file_mtime = str(file_stats.st_mtime)
    file_size = str(file_stats.st_size)

    # MSGCODE, fileName, File_mTime, File_size, forcemode
    reply = "SEND_FILE" + delimiter + file_name + delimiter + file_mtime + delimiter + file_size + delimiter + force
    send_msg(socket, str.encode(reply))
    returnedData = recv_msg(socket)
    data_array = returnedData.decode('utf-8').split(delimiter)
    if (data_array[0] == "WAITING_FOR_FILE"):
        print("File SENDING request was received, submit via port: " + data_array[1])
        network_send_file(file_base_path, file_name, host, int(data_array[1]))
        # break
    elif(data_array[0] == "FILE_ALREADY_EXISTS"):
        print("File SENDING request was received, but file isn't newer")
        # break
    else:
        try:
            answercode = returnedData.decode('utf-8')
        except:
            raise Exception("ERROR: SENDING file; No answer at all (yet)")
        raise Exception("SENDING file; Unrecognized answer: " + returnedData.decode('utf-8'))
    
        #print("Reply: " + returnedData.decode('utf-8'))
    print("")

def request_file(socket, file_base_path, file_name, host, force):
    if(os.path.isfile(file_base_path + file_name)):
        file_stats = os.stat(file_base_path + file_name)
        file_mtime = str(file_stats.st_mtime)
        file_size = str(file_stats.st_size)
    else:
        file_stats = []
        file_mtime = "0"
        file_size = "0"
    
    # MSGCODE, fileName, File_mTime, File_size, forcemode
    reply = "REQUEST_FILE" + delimiter + file_name + delimiter + file_mtime + delimiter + file_size + delimiter + force
    send_msg(socket, str.encode(reply))
    returnedData = recv_msg(socket)
    data_array = returnedData.decode('utf-8').split(delimiter)

    # handle answer to file request
    if (data_array[0] == "START_WAITING_FOR_REQUESTED_FILE"):
        file_transfer_port = acquire_port()
        print("File transfer new port: " + str(file_transfer_port)) 
        file_transfer_port_string = str(file_transfer_port)
        server_file_mtime = data_array[1]
        server_file_size = data_array[2]
        print("File REQUEST request was received, receiving via port: " + file_transfer_port_string)
        # MSGCODE, fileName, host, port
        reply = "READY_TO_RECEIVE_FILE" + delimiter + file_name + delimiter + file_transfer_port_string
        send_msg(socket, str.encode(reply))
        network_receive_file(file_base_path, file_name, file_size, local_host, file_transfer_port)
        release_port(file_transfer_port)
        # break
    elif(data_array[0] == "NO_UPDATED_FILE_AVAILABLE"):
        print("File REQUEST request was received, but file isn't newer")
        # break
    elif(data_array[0] == "FILE_DOES_NOT_EXIST"):
        raise Exception("File REQUEST request was received, but file: " + file_base_path + file_name + " does not exist")
        # break
    else:
        try:
            answercode = returnedData.decode('utf-8')
            # raise Exception("REQUEST file; Unrecognized answer: " + returnedData.decode('utf-8'))
        except:
            raise Exception("ERROR: REQUEST file; No answer at all (yet)")
        raise Exception("REQUEST file; Unrecognized answer: " + returnedData.decode('utf-8'))
        #print("Reply: " + returnedData.decode('utf-8'))
    print("")

skt = socket.socket()
try:
    skt.connect((target_host, target_port))
except:
    raise Exception("Connection error. Tried to reach: " + str(target_host) + ":" + str(target_port)) 

returnedData = recv_msg(skt)
print("CONNECTION: " + returnedData.decode('utf-8'))
if (returnedData.decode('utf-8') == "ACC"):
    send_file(skt, "/path/to/Desktop/", "BB.png", target_host, target_port, "0")
    send_file(skt, "/path/to/Desktop/", "BB.png", target_host, target_port, "FORCE")
    request_file(skt, "/path/to/Desktop/IN/", "UnreachableArea.png", target_host, "0")
    request_file(skt, "/path/to/Desktop/IN/", "UnreachableArea.png", target_host, "FORCE")
else:
    print("Error: Unrecognized answer: '" + returnedData.decode("utf-8") +"'")
print("Closing socket")
skt.close()



options_string = "-i 150 -p 100 -N [0,0,1] -h 2.0"
path_to_executable = "/path/to/matryoshka/Release/matryoshka"
path_to_outer_file = "/path/to/Assets/S60-elevated.stl"
path_to_inner_file = "/path/to/Assets/Egg.stl"

shellCommand = path_to_executable + " " + options_string + " " + path_to_outer_file + " " + path_to_inner_file


async def run_command(*args):

    # Create subprocess
    process = await asyncio.create_subprocess_exec(
        *args, stdout=asyncio.subprocess.PIPE, stderr=asyncio.subprocess.STDOUT
    )

    # Status
    print("Started: %s, pid=%s" % (args, process.pid), flush=True)

    # Wait for the subprocess to finish
    stdout, stderr = await process.communicate()

    # Progress
    if process.returncode == 0:
        print(
            "Done: %s, pid=%s, result: %s"
            % (args, process.pid, stdout.decode().strip()),
            flush=True,
        )
    else:
        print(
            "Failed: %s, pid=%s, result: %s"
            % (args, process.pid, stderr.decode().strip()),
            flush=True,
        )

    # Result
    result = stdout.decode().strip()

    # Return stdout
    return result

# run_command(shellCommand)

p = subprocess.Popen(shellCommand, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
for line in p.stdout.readlines():
    print(line)

retval = p.wait()
print(retval)
streamdata = p.communicate()[0]
rc = p.returncode
print(rc)

