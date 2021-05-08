import struct
import socket
import os

def send_msg(skt, msg):
    # Prefix each message with a 4-byte length (network byte order)
    msg = struct.pack('>I', len(msg)) + msg
    skt.sendall(msg)

def recv_msg(skt):
    # Read message length and unpack it into an integer
    raw_msglen = recv_all(skt, 4)
    if not raw_msglen:
        return None
    msglen = struct.unpack('>I', raw_msglen)[0]
    # Read the message data
    return recv_all(skt, msglen)

def recv_all(skt, n):
    # Helper function to recv n bytes or return None if EOF is hit
    data = bytearray()
    while len(data) < n:
        packet = skt.recv(n - len(data))
        if not packet:
            return None
        data.extend(packet)
    return data

def network_receive_file(base_path_fo_file, file_name, file_size, file_host, file_port):
    path_to_file = base_path_fo_file + file_name
    receiving = True
    connection_open = True

    # open separate connection for file transfer
    file_skt = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        file_skt.bind(('', file_port))
    except socket.error as e:
        print(str(e))
    file_skt.listen(10) # Accepts only 1 connection. // safeguard against connections from the internet that we hope to 

    # wait to receive file
    while(connection_open):
        file_conn, addr = file_skt.accept()
        # print('file transfer connection to: '+addr[0]+':'+str(addr[1]))
        r_file = open(path_to_file, 'wb') #open in binary
        while(receiving):       
        # receive data and write it to file
            chunk = file_conn.recv(1024)
            while(chunk):
                r_file.write(chunk)
                chunk = file_conn.recv(1024)
            r_file.close()
            if not chunk: # gentle break to avoid throwing away the last chunk somehow
                receiving = False
        file_conn.close()
        if not chunk: # gentle break
            connection_open = False
    file_skt.close()
    check_size = str(os.stat(path_to_file).st_size) # check local file size after file transfer
    if (check_size == file_size):
        # print("File transfer successfull")
        return 1
    else:
        # print("File transfer error; expected size: " + str(file_size) + ", received size: " + str(check_size))
        return 0
    # return r_file

def network_send_file(base_path_fo_file, file_name, file_host, file_port):
    path_to_file = base_path_fo_file + file_name
    file_skt = socket.socket()
    file_skt.connect((file_host, file_port))

    file_to_send = open(base_path_fo_file + file_name, "rb")

    chunk = file_to_send.read(1024)
    while (chunk):
        file_skt.send(chunk)
        chunk = file_to_send.read(1024)
    file_to_send.close()
    file_skt.close()