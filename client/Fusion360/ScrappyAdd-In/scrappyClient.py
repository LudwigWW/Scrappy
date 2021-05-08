import socket
import os
import random
import asyncio
import time
import subprocess
import platform
from enum import Enum
from .scrappyNetwork import *

# class clientOperations(Enum):
#     sendModel = 1
#     scrapTest = 2
#     meshGeneration = 3
#     receiveScrap = 4
#     receiveModel = 5
#     getPaddedVolume = 6

class scrappyClientClass():
    def __init__(self, host_in, port_in, scrappy_library_path, scrappy_library_simple_path, scrappy_library_full_path, scrappy_export_path, scrappy_import_path, debugMode):
        from threading import Lock

        self.library_path = scrappy_library_path
        self.library_simple_path = scrappy_library_simple_path
        self.library_full_path = scrappy_library_full_path
        self.export_path = scrappy_export_path
        self.import_path = scrappy_import_path
        self.delimiter = "<::>"
        self.local_host = ""
        self.target_host = host_in
        self.target_port = port_in
        self.debugMode = debugMode

        self.ports_in_use = [port_in]
        self.port_mutex = Lock()

        # Debug
        self.options_string = "-e 0.25 -i 400 -p 400 -N [0,0,1] -h 0.6 -s 0.2"
        #self.options_string = "-e 0.4 -i 150 -p 100 -N [0,0,1] -h 2.0 -s 0.05"
        self.path_to_executable = "/path/to/matryoshka/Release/matryoshka"
        
        self.path_to_outer_file = "/path/to/Assets/S60-elevated.stl"
        self.path_to_inner_file = "/path/to/Assets/Egg.stl"
        self.shellCommand = self.path_to_executable + " " + self.options_string + " " + self.path_to_outer_file + " " + self.path_to_inner_file

    # string to array #TODO: use JSON or send a file
    def parse_array_string(self, array_string):
        array_string = array_string.replace("  ", " ").replace("  ", " ").replace("[", "").replace("]", "").replace("'", "")
        array = array_string.split(", ")
        for i in range(0, len(array)):
            array[i] = array[i].split(" ")
        return array

    # selecting unused port
    def acquire_port(self):
        self.port_mutex.acquire()
        try:
            file_port = random.randrange(46337,46997,1)
            while(file_port in self.ports_in_use):
                file_port = random.randrange(46337,46997,1)
            self.ports_in_use.append(file_port)
            if (self.debugMode):
                print("New port acquired, ports in use: " + str(self.ports_in_use))
        finally:
            self.port_mutex.release()
            return file_port

    # release port when no longer in use
    def release_port(self, port):
        self.port_mutex.acquire()
        try:
            if(port in self.ports_in_use):
                self.ports_in_use.remove(port)
            if (self.debugMode):
                print("Port released, ports in use: " + str(self.ports_in_use))
        finally:
            self.port_mutex.release()

    def remote_matryoshka(self, socket, options_string, outer_file_name, inner_file_name, outer_hq_name, inner_hq_name, target_host, target_port):
        message = "EXECUTE_MATRYOSHKA" + self.delimiter + options_string + self.delimiter + outer_file_name + self.delimiter + inner_file_name + self.delimiter + outer_hq_name + self.delimiter + inner_hq_name
        send_msg(socket, str.encode(message))
        returnedData = recv_msg(socket)
        data_array = returnedData.decode('utf-8').split(self.delimiter)
        return_info = []
        if (data_array[0] == "MATRYOSHKA_SUCCESS"):
            return_info = self.parse_array_string(data_array[3]) # parsing again here is meh, but I didn't want to send 8 strings, or a new file just for this.
            if (self.debugMode):
                print("Matroshka was successful!")
                for element in return_info:
                    print(element)
                print("Output:")
                print(data_array[2])
        elif (data_array[0] == "MATRYOSHKA_UNSUCCESSFUL"):
            if (self.debugMode):
                print("Matryoshka failed...")
                print("Output:")
                print(data_array[2])
        elif (data_array[0] == "MATRYOSHKA_ERROR_STL"):
            return_info = self.parse_array_string(data_array[3])
            if (self.debugMode):
                print("Matroshka failed, but found position! ")
                for element in return_info:
                    print(element)
                print("Output:")
                print(data_array[2])
        elif (data_array[0] == "MATRYOSHKA_ERROR_UNKOWN"):
            if (self.debugMode):
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
        if (self.debugMode):
            print("")
        return [data_array[1], return_info]

    def remote_command(self, socket, path_to_executable, options_string, path_to_outer_file, path_to_inner_file, target_host, target_port):
        message = "EXECUTE_COMMAND" + self.delimiter + path_to_executable + self.delimiter + options_string + self.delimiter + path_to_outer_file + self.delimiter + path_to_inner_file
        send_msg(socket, str.encode(message))
        returnedData = recv_msg(socket)
        data_array = returnedData.decode('utf-8').split(self.delimiter)
        if (data_array[0] == "COMMAND_RESULT"):
            if (self.debugMode):
                print("Command returned with: " + data_array[1] + " and output:\n" + str(data_array[2]))
        else:
            try:
                answercode = returnedData.decode('utf-8')
            except:
                raise Exception("ERROR: COMMAND; No answer at all (yet)")
            raise Exception("COMMAND; Unrecognized answer: " + returnedData.decode('utf-8'))
        if (self.debugMode):
            print("")


    def send_file(self, socket, file_base_path, file_name, host, port, force):
        # while (True):
        if(not os.path.isfile(file_base_path + file_name)):
            raise Exception("File to send does not exist at path: " + file_base_path + file_name) 
        file_stats = os.stat(file_base_path + file_name)
        file_mtime = str(file_stats.st_mtime)
        file_size = str(file_stats.st_size)

        # MSGCODE, fileName, File_mTime, File_size, forcemode
        message = "SEND_FILE" + self.delimiter + file_name + self.delimiter + file_mtime + self.delimiter + file_size + self.delimiter + force
        send_msg(socket, str.encode(message))
        returnedData = recv_msg(socket)
        data_array = returnedData.decode('utf-8').split(self.delimiter)
        if (data_array[0] == "WAITING_FOR_FILE"):
            if (self.debugMode):
                print("File SENDING request was received, submit via port: " + data_array[1])
            network_send_file(file_base_path, file_name, host, int(data_array[1]))
            # break
        elif(data_array[0] == "FILE_ALREADY_EXISTS"):
            if (self.debugMode):
                print("File SENDING request was received, but file isn't newer")
            # break
        else:
            try:
                answercode = returnedData.decode('utf-8')
            except:
                raise Exception("ERROR: SENDING file; No answer at all (yet)")
            raise Exception("SENDING file; Unrecognized answer: " + returnedData.decode('utf-8'))
        
            #print("Reply: " + returnedData.decode('utf-8'))
        if (self.debugMode):
            print("")

    def request_file(self, socket, file_base_path, file_name, host, force):
        if(os.path.isfile(file_base_path + file_name)):
            file_stats = os.stat(file_base_path + file_name)
            file_mtime = str(file_stats.st_mtime)
            file_size = str(file_stats.st_size)
        else:
            file_stats = []
            file_mtime = "0"
            file_size = "0"
        
        # MSGCODE, fileName, File_mTime, File_size, forcemode
        message = "REQUEST_FILE" + self.delimiter + file_name + self.delimiter + file_mtime + self.delimiter + file_size + self.delimiter + force
        send_msg(socket, str.encode(message))
        returnedData = recv_msg(socket)
        data_array = returnedData.decode('utf-8').split(self.delimiter)

        # handle answer to file request
        if (data_array[0] == "START_WAITING_FOR_REQUESTED_FILE"):
            file_transfer_port = self.acquire_port()
            if (self.debugMode):
                print("File transfer new port: " + str(file_transfer_port)) 
            file_transfer_port_string = str(file_transfer_port)
            server_file_mtime = data_array[1]
            server_file_size = data_array[2]
            if (self.debugMode):
                print("File REQUEST request was received, receiving via port: " + file_transfer_port_string)
            # MSGCODE, fileName, host, port
            message = "READY_TO_RECEIVE_FILE" + self.delimiter + file_name + self.delimiter + file_transfer_port_string
            send_msg(socket, str.encode(message))
            network_receive_file(file_base_path, file_name, file_size, self.local_host, file_transfer_port)
            self.release_port(file_transfer_port)
            # break
        elif(data_array[0] == "NO_UPDATED_FILE_AVAILABLE"):
            if (self.debugMode):
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
        if (self.debugMode):
            print("")

    def start_client(self, operation, scrapItem, modelObj, send_file_name = "", send_file_folder_type = ""):
        # Prepare connection by (for some reason) pinging the server first, or the socket might not find the server in the network
        os_param = '-n' if platform.system().lower()=='windows' else '-c'

        p = subprocess.Popen("ping " + os_param + " 1 " + self.target_host, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        return_value = p.wait()


        file_lookup_path = self.export_path # default is sending the model from export
        # change send file lookup location
        if send_file_folder_type == "LibrarySimplified":
            file_lookup_path = self.library_simple_path
        elif send_file_folder_type == "LibraryFull":
            file_lookup_path = self.library_full_path
        elif send_file_folder_type == "Library":
            file_lookup_path = self.library_path

        # Open the socket
        skt = socket.socket()
        try:
            skt.connect((self.target_host, self.target_port))
        except:
            raise Exception("Connection error. Tried to reach: " + str(self.target_host) + ":" + str(self.target_port)) 

        # Socket connection status
        returnedData = recv_msg(skt)
        if (self.debugMode):
            print("CONNECTION: " + returnedData.decode('utf-8') + ": " + str(operation))

        # Start operation
        results_array = []
        if (returnedData.decode('utf-8') == "ACC"):
            if (operation == 'clientOperations.sendSTL'):
                # Send current model
                self.send_file(skt, file_lookup_path, send_file_name, self.target_host, self.target_port, "0")

            elif (operation == 'clientOperations.receiveModel'):
                # Get adjusted model file
                adjusted_file_name = modelObj.name[:-9] + '_' + scrapItem.name[:-9] + '_hqModel.stl'
                self.request_file(skt, self.import_path, adjusted_file_name, self.target_host, "0")

            elif (operation == 'clientOperations.receiveScrap'):
                # Get repositioned scrap file
                adjusted_scrap_file_name = modelObj.name[:-9] + '_' + scrapItem.name[:-9] + '.stl'
                self.request_file(skt, self.import_path, adjusted_scrap_file_name, self.target_host, "0")

            elif (operation == 'clientOperations.scrapTest'):
                # Send scrap file
                self.send_file(skt, file_lookup_path, send_file_name, self.target_host, self.target_port, "0")
                # Start matryoshka in position only mode
                results_array = self.remote_matryoshka(skt, self.options_string + " -z 1 ", modelObj.simpleName, send_file_name, "", "", self.target_host, self.target_port)
                if (results_array[0] == "0"):
                    # Get repositioned scrap file
                    adjusted_scrap_file_name = modelObj.name[:-9] + '_' + scrapItem.name[:-9] + '.stl'
                    self.request_file(skt, self.import_path, adjusted_scrap_file_name, self.target_host, "0")

            elif (operation == 'clientOperations.meshGeneration'):
                # Start matryoshka in mesh generation only mode
                results_array = self.remote_matryoshka(skt, self.options_string + " -z 2 -x " + scrapItem.contFile, modelObj.blobName, scrapItem.simpleName, modelObj.name, scrapItem.name, self.target_host, self.target_port)
                if (results_array[0] == "0"):
                    # # Get adjusted model file
                    # adjusted_file_name = modelObj.name[:-9] + '_' + scrapItem.name[:-9] + '_EmptyModel.stl' # for preview
                    # self.request_file(skt, self.import_path, adjusted_file_name, self.target_host, "0")
                    print("Blob mesh generation successful for: " + str(modelObj.blobName) + ", " + str(scrapItem.simpleName))

            elif (operation == 'clientOperations.getScrappedVolume'):
                # Getting scrap, including the padded removed volume around it for the insertion paths
                adjusted_scrapped_scrap_file_name = modelObj.name[:-9] + '_' + scrapItem.name[:-9] + '_ScrappedVolume.stl' # for slicing
                self.request_file(skt, self.import_path, adjusted_scrapped_scrap_file_name, self.target_host, "0")

            elif (operation == 'clientOperations.testing'):
                # Do the whole thing at once for testing purposes
                results_array = self.remote_matryoshka(skt, self.options_string + " -z 0 ", modelObj.name, scrapItem.name, "", "", self.target_host, self.target_port)
                adjusted_file_name = modelObj.name[:-9] + '-00-99.stl'
                adjusted_scrap_file_name = modelObj.name[:-9] + '-01-0.stl'
                self.request_file(skt, self.import_path, adjusted_file_name, self.target_host, "0")
                self.request_file(skt, self.import_path, adjusted_scrap_file_name, self.target_host, "0")
                
            # Hardcoed one-off debug test connection
            # send_file(skt, "/path/to/Desktop/", "BB.png", target_host, target_port, "FORCE")
            # request_file(skt, "/path/to/Desktop/IN/", "UnreachableArea.png", target_host, "0")
            # request_file(skt, "/path/to/Desktop/IN/", "UnreachableArea.png", target_host, "FORCE")
            # remote_command(skt, path_to_executable, options_string, path_to_outer_file, path_to_inner_file, target_host, target_port)
            # remote_matryoshka(skt, self.options_string, self.path_to_outer_file, self.path_to_inner_file, self.target_host, self.target_port)
            
        else:
            try:
                answercode = returnedData.decode('utf-8')
            except:
                raise Exception("ERROR: Initial connection, can't decode answer")
            raise Exception("Error: Unrecognized answer to initial connection: '" + returnedData.decode("utf-8") +"'")
        if (self.debugMode):
            print("Closing socket")
        skt.close()
        return results_array

