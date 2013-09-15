import socket
import sys
from Zeroconf import *
import time

class mDNSSniffer:
  def __init__(self):
      self.record = None
  
  def updateRecord(self, zeroconf, now, record):
    print "got record:"
    print "  type: ", record.type
    print "  name: ", record.name
    if record.type == 33:
      print "  server: ", record.server
      print "  port: ", record.port
      print "  weight: ", record.weight
      print "  priority: ", record.priority
    elif record.type == 1:
      print "  addr: ", socket.inet_ntoa(record.address)
      
r = Zeroconf('192.168.1.146')
l = mDNSSniffer()
#r.addListener(l, None)
#time.sleep(5)

print "requesting brewbit service info"

type = "_device-info._udp.local."
name = "brewbit-model-t._device-info._udp.local."
info = r.getServiceInfo(type, name, 6000)

print "service info:", info

r.close()

# def connect_to_debug_server():
    # # Create a TCP/IP socket
    # sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    # # Bind the socket to the port
    # server_address = ('', 35287)
    # print 'Listening for connections on %s port %s' % server_address
    # sock.bind(server_address)

    # # Listen for incoming connections
    # sock.listen(1)

    # while True:
        # # Wait for a connection
        # print 'Waiting for a connection'
        # connection, client_address = sock.accept()

        # try:
            # print 'Connection from', client_address

            # # Receive the data in small chunks and retransmit it
            # while True:
                # data = connection.recv(16)
                # if data:
                    # print data
                # else:
                    # print 'No more data from', client_address
                    # break
        # except Exception:
            # print 'Dropped connection with', client_address
            # pass
        # finally:
            # # Clean up the connection
            # connection.close()
            