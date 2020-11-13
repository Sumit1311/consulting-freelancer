import zlib
from sys import *
from socket import *

PORT = int(argv[1])
HOST = ''

serverSocket = socket(AF_INET, SOCK_DGRAM)
serverSocket.bind((HOST, PORT))
expectedseq = 0
prev_msg = ''
while True:
    while True:
        msg = ''
        msg, clientaddr = serverSocket.recvfrom(4000)
        msg = msg.decode("utf-8")
        other_seq = str((expectedseq + 1) % 2)
        if len(msg) == 0:
            break
        message = msg.split('|')
        if (len(message) != 3):
            reply = "ACK" + other_seq
            serverSocket.sendto(reply.encode(), clientaddr)  #send a nak
            continue
        recv_checksum = message[0]
        seq = message[1]
        content = message[2]
        checksum = str(zlib.crc32(content.encode("utf-8")))
        if checksum == recv_checksum:
            reply = "ACK" + seq
            serverSocket.sendto(reply.encode(), clientaddr)
            #print("Sending : " + reply)
            if seq == str(expectedseq):
                print(content, end="")
                expectedseq = (expectedseq + 1) % 2
                continue
        else:
            reply = "ACK" + other_seq
            serverSocket.sendto(reply.encode(), clientaddr)
        '''
        try:
            message[0] = int(message[0])
        except ValueError:
            serverSocket.sendto(msg.encode("utf-8"), clientaddr)
            continue
        if message[0] != expectedseq:  #if wrong seq, break
            print("Wrong sequence number")
            #send ACK, but don't print and drop the packet
            serverSocket.sendto(msg.encode("utf-8"), clientaddr)
            continue
        
        if checksum != message[1]:
            print("Checksum mismatch")
            continue
        payload = str(expectedseq)
        expectedseq = (expectedseq + 1) % 2
        serverSocket.sendto(payload.encode("utf-8"), clientaddr)
        print(message[2], end="")
        '''