# connect to openocd server
target remote:3333

# load the binary
monitor reset halt
load

# break at main
break main
continue
