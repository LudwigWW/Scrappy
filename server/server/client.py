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
target_host =  "10.42.0.46"
target_port = 12397

ports_in_use = [47556]
port_mutex = Lock()


options_string = "-i 150 -p 100 -N [0,0,1] -h 2.0"
path_to_executable = "/path/to/matryoshka/Release/matryoshka"
path_to_outer_file = "/path/to/Assets/S60-elevated.stl"
path_to_inner_file = "/path/to/Assets/Egg.stl"

shellCommand = path_to_executable + " " + options_string + " " + path_to_outer_file + " " + path_to_inner_file


# string to array #TODO: use JSON or send a file
def parse_array_string(array_string):
    array_string = array_string.replace("  ", " ").replace("  ", " ").replace("[", "").replace("]", "").replace("'", "")
    array = array_string.split(", ")
    for i in range(0, len(array)):
        array[i] = array[i].split(" ")
    return array

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

def remote_matryoshka(socket, options_string, path_to_outer_file, path_to_inner_file, target_host, target_port):
    message = "EXECUTE_MATRYOSHKA" + delimiter + options_string + delimiter + path_to_outer_file + delimiter + path_to_inner_file
    send_msg(socket, str.encode(message))
    returnedData = recv_msg(socket)
    data_array = returnedData.decode('utf-8').split(delimiter)
    if (data_array[0] == "MATRYOSHKA_SUCCESS"):
        position_info = parse_array_string(data_array[3]) # parsing again here is meh, but I didn't want to send 8 strings, or a new file just for this.
        print("Matroshka was successful!")
        for element in position_info:
            print(element)
    elif (data_array[0] == "MATRYOSHKA_UNSUCCESSFUL"):
        print("Matryoshka failed...") 
    elif (data_array[0] == "MATRYOSHKA_ERROR_STL"):
        position_info = parse_array_string(data_array[3])
        print("Matroshka failed, but found position! ")
        for element in position_info:
            print(element)
        print("Output:")
        print(data_array[2])
    elif (data_array[0] == "MATRYOSHKA_ERROR_UNKOWN"):
        print("Matryoshka died with error code: " + data_array[1])
        print("Output:")
        print(data_array[2])     
        #TODO: Handle error case
    else:
        try:
            answercode = returnedData.decode('utf-8')
        except:
            raise Exception("ERROR: MATRYOSHKA; No answer at all (yet)")
        raise Exception("MATRYOSHKA; Unrecognized answer: " + returnedData.decode('utf-8'))
    print("")

def remote_command(socket, path_to_executable, options_string, path_to_outer_file, path_to_inner_file, target_host, target_port):
    message = "EXECUTE_COMMAND" + delimiter + path_to_executable + delimiter + options_string + delimiter + path_to_outer_file + delimiter + path_to_inner_file
    send_msg(socket, str.encode(message))
    returnedData = recv_msg(socket)
    data_array = returnedData.decode('utf-8').split(delimiter)
    if (data_array[0] == "COMMAND_RESULT"):
        print("Command returned with: " + data_array[1] + " and output:\n" + str(data_array[2]))
    else:
        try:
            answercode = returnedData.decode('utf-8')
        except:
            raise Exception("ERROR: COMMAND; No answer at all (yet)")
        raise Exception("COMMAND; Unrecognized answer: " + returnedData.decode('utf-8'))
    print("")


def send_file(socket, file_base_path, file_name, host, port, force):
    # while (True):
    if(not os.path.isfile(file_base_path + file_name)):
        raise Exception("File to send does not exist at path: " + file_base_path + file_name) 
    file_stats = os.stat(file_base_path + file_name)
    file_mtime = str(file_stats.st_mtime)
    file_size = str(file_stats.st_size)

    # MSGCODE, fileName, File_mTime, File_size, forcemode
    message = "SEND_FILE" + delimiter + file_name + delimiter + file_mtime + delimiter + file_size + delimiter + force
    send_msg(socket, str.encode(message))
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
    message = "REQUEST_FILE" + delimiter + file_name + delimiter + file_mtime + delimiter + file_size + delimiter + force
    send_msg(socket, str.encode(message))
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
        message = "READY_TO_RECEIVE_FILE" + delimiter + file_name + delimiter + file_transfer_port_string
        send_msg(socket, str.encode(message))
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
    send_file(skt, "/path/to/debug/folder/", "BB.png", target_host, target_port, "0")
    # send_file(skt, "/path/to/debug/folder/", "BB.png", target_host, target_port, "FORCE")
    # request_file(skt, "/path/to/debug/folder/IN/", "UnreachableArea.png", target_host, "0")
    # request_file(skt, "/path/to/debug/folder/IN/", "UnreachableArea.png", target_host, "FORCE")
    # remote_command(skt, path_to_executable, options_string, path_to_outer_file, path_to_inner_file, target_host, target_port)
    remote_matryoshka(skt, options_string, path_to_outer_file, path_to_inner_file, target_host, target_port)
    
else:
    print("Error: Unrecognized answer: '" + returnedData.decode("utf-8") +"'")
print("Closing socket")
skt.close()






async def run_command(*args):

    # Create subprocess
    process = await asyncio.create_subprocess_exec(
        *args, shell=True, stdout=asyncio.subprocess.PIPE, stderr=asyncio.subprocess.STDOUT
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
async def thread_it():
    await asyncio.gather(run_command(shellCommand))

# a, b = asyncio.run(thread_it())

# loop = asyncio.get_event_loop()
# loop.run_until_complete(asyncio.wait([run_command(shellCommand)]))

# print(a)
# print("..........")
# print(b)


