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
            return svc.address

def transfer_file(addr):
    s = socket.create_connection((addr, 5000))
    while True:
        s.send("hello world!")
        time.sleep(3)

print "Waiting for Model-T Service Advertisement..."
# addr = find_service_addr()
transfer_file("192.168.1.132")
print "Received Model-T Service Advertisement from", socket.inet_ntoa(addr)
