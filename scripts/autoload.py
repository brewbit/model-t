import os
import serial

s = serial.Serial('/dev/ttyUSB0', 9600)

while True:
    # command arduino to wait for board connection
    print "Waiting for board connection"
    s.write('r')
    
    # wait for arduino to indicate board connected
    if s.read(1) != '1':
        continue
    
    # command arduino to reset board in boot mode
    print "Entering BOOT mode"
    s.write('b')
    
    # wait for arduino to confirm
    if s.read(1) != '1':
        continue
    
    # start loading via DFU
    print "Starting download"
    if os.system('dfu-util -a 0 -t 2048 -D build/all.dfu') != 0:
        continue
    
    # command arduino to reset board in run mode
    print "Entering RUN mode"
    s.write('r')
    
    # wait for arduino to confirm
    if s.read(1) != '1':
        continue
    
    while True:
        print "Waiting for board disconnection"
        # command arduino to wait for board disconnect
        s.write('d')
    
        # wait for arduino to indicate board disconnected
        if s.read(1) == '1':
            break
