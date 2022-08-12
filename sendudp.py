<<<<<<< HEAD


import socket

#sock = socket.socket()

#sock.bind('', 5000)

udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

udp_socket.bind(('localhost', 9000))

trgt_addr = ('localhost', 3000)

#while True:

print('send!')

udp_socket.sendto(b'hello bro!', trgt_addr)


=======


import socket

#sock = socket.socket()

#sock.bind('', 5000)

udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

udp_socket.bind(('localhost', 9000))

trgt_addr = ('localhost', 3000)

#while True:

print('send!')

udp_socket.sendto(b'hello bro!', trgt_addr)


>>>>>>> 6c2aebdebf8275204430594543fe570651592429
udp_socket.close()