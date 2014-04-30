import sys
import binascii
import struct

app_file = open(sys.argv[2], "rb")
app_crc = (binascii.crc32(app_file.read()) & 0xFFFFFFFF)
app_size = app_file.tell()
app_file.close()

hdr_file = open(sys.argv[1], "r+b")
hdr_file.seek(0x14)
hdr_file.write(struct.pack("<L", app_size))
hdr_file.write(struct.pack("<L", app_crc))
hdr_file.close()
