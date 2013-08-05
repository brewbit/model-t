import socket
import sys

# Create a TCP/IP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

# Bind the socket to the port
server_address = ('', 35287)
print 'Listening for connections on %s port %s' % server_address
sock.bind(server_address)

# Listen for incoming connections
sock.listen(1)

while True:
    # Wait for a connection
    print 'Waiting for a connection'
    connection, client_address = sock.accept()

    try:
        print 'Connection from', client_address

        # Receive the data in small chunks and retransmit it
        while True:
            data = connection.recv(16)
            if data:
                print data
            else:
                print 'No more data from', client_address
                break
    except Exception:
        print 'Dropped connection with', client_address
        pass
    finally:
        # Clean up the connection
        connection.close()