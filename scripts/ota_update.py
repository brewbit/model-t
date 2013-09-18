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

def transfer_file(s, filename):
    f = open(filename, "rb")
    s.sendall(f.read(256))
    

print "Waiting for Model-T service announcement...",
addr = find_service_addr()
print addr

print "Connecting to Model-T service...",
s = socket.create_connection((addr, 5000))
print "OK"

print "Sending app header...",
transfer_file(s, "build/app_mt/app_mt_hdr.bin")
print "OK"

print "Waiting for header download confirmation...",
s.recv(1)
print "OK"

print "Sending app image...",
transfer_file(s, "build/app_mt/app_mt_app.bin")
print "OK"

print "Waiting for image download confirmation...",
s.recv(1)
print "OK"
