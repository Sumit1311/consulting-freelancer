import zlib
from sys import *
from socket import *

#packets = []

#ACK format : "ACK 1"
SEGMENT_SIZE = 50


def create_packet(message, seq_num, checksum):
    # format : (seq num)'|'(checksum)'|'(packet data)
    #need to include something to indicate end of packet. Probably double space
    msg = ""
    msg = msg + str(seq_num)
    msg = msg + '|'
    #checksum = str(zlib.crc32(message.encode()))
    msg = msg + checksum
    msg = msg + '|'
    msg = msg + message
    #print("Packet is : " + msg)
    return msg


PORT = int(argv[1])
HOST = ''
clientSocket = socket(AF_INET, SOCK_DGRAM)
seq_num = 0
while True:
    send_till = 0
    message = ''
    message = stdin.readline()
    #message = message.strip('\r\n')
    if len(message) == 0:
        #print("Break")
        break
    while send_till < len(message):
        if send_till + SEGMENT_SIZE > len(message):
            segment = message[send_till:]
        else:
            segment = message[send_till:send_till + SEGMENT_SIZE]
        send_till += SEGMENT_SIZE
        ack_recv = False
        while not ack_recv:
            checksum = str(zlib.crc32(segment.encode("utf-8")))
            msg = checksum + "|" + str(seq_num) + "|" + segment
            clientSocket.settimeout(0.05)
            clientSocket.sendto(msg.encode(), (HOST, PORT))
            #print("Sending : " + msg)
            try:
                print("trying to receive")
                recv_ack, address = clientSocket.recvfrom(64)
            except timeout:
                print("Timeout")
                continue
            else:
                recv_ack = recv_ack.decode("utf-8")
                ack_seq = recv_ack[3:]
                if ack_seq == str(seq_num):
                    ack_recv = True
                    seq_num = (seq_num + 1) % 2
