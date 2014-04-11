import os
import serial
import time

s = serial.Serial('/dev/ttyUSB0', 9600, timeout=0.5)

while True:
    time.sleep(0.2)

    # command arduino to wait for board connection
    print "Waiting for board connection"
    s.write('r')
    
    time.sleep(0.4)

    # wait for arduino to indicate board connected
    if s.read(1) != '1':
        continue

    time.sleep(0.4)
    
    # command arduino to reset board in boot mode
    print "Entering BOOT mode"
    s.write('b')

    time.sleep(0.4)
    
    # wait for arduino to confirm
    if s.read(1) != '1':
        continue

    time.sleep(0.4)
    
    # start loading via DFU
    print "Starting download"
    if os.system('dfu-util -a 0 -t 2048 -D build/all.dfu') != 0:
        continue

    time.sleep(0.4)
    
    # command arduino to reset board in run mode
    print "Entering RUN mode"
    s.write('r')

    time.sleep(0.4)
    
    # wait for arduino to confirm
    if s.read(1) != '1':
        continue
    
    time.sleep(0.4)

    while True:
        print "Waiting for board disconnection"
        # command arduino to wait for board disconnect
        s.write('d')
    
        time.sleep(0.4)

        # wait for arduino to indicate board disconnected
        if s.read(1) == '1':
            break
