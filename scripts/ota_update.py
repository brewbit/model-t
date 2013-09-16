import socket
import sys
from Zeroconf import *
import time


BREWBIT_SERVICE_TYPE = "_device-info._udp.local."
BREWBIT_SERVICE_NAME = "brewbit-model-t." + BREWBIT_SERVICE_TYPE

def find_service_addr():
    r = Zeroconf()

    while True:
        svc = r.getServiceInfo(BREWBIT_SERVICE_TYPE, BREWBIT_SERVICE_NAME, 6000)

        if svc:
            r.close()
            return socket.inet_ntoa(svc.address)

def transfer_file(addr):
    s = socket.create_connection((addr, 5000))
    while True:
        s.send("hello world!\r\n")
        time.sleep(1)

print "Waiting for Model-T service announcement..."
addr = find_service_addr()
print "  Received announcement from", addr
print "Starting file upload"
transfer_file(addr)
