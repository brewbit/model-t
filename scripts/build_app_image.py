import sys
import binascii
import struct
import math

class SxfsFile:
    def __init__(self, filename, id):
        f = open(filename, "rb")
        
        self.id = id
        self.data = f.read()
        self.size = f.tell()
        
        f.close()

class SxfsImageBuilder:
    def __init__(self):
        self.files = []
    
    def add_file(self, file):
        self.files.append(file)
        return self
    
    def part_data(self):
        data_offset = 16 + sum([16 for f in self.files])
        
        buf = ""
        
        for f in self.files:
            buf = buf + struct.pack("<L", f.id)
            buf = buf + struct.pack("<L", data_offset)
            buf = buf + struct.pack("<L", f.size)
            buf = buf + struct.pack("<L", (binascii.crc32(f.data) & 0xFFFFFFFF))
            data_offset = data_offset + f.size
        
        for f in self.files:
            buf = buf + f.data
        
        return buf
    
    def serialize(self):
        part_data = self.part_data()
        buf = "SXFS"
        buf = buf + struct.pack("<L", sum([f.size + 16 for f in self.files]))
        buf = buf + struct.pack("<L", len(self.files))
        buf = buf + struct.pack("<L", (binascii.crc32(part_data) & 0xFFFFFFFF))
        buf = buf + part_data
        
        return buf
        

app_file = open(sys.argv[2], "rb")
app_crc = (binascii.crc32(app_file.read()) & 0xFFFFFFFF)
app_size = app_file.tell()

hdr_file = open(sys.argv[1], "r+b")
hdr_file.seek(0x14)
hdr_file.write(struct.pack("<L", app_size))
hdr_file.write(struct.pack("<L", app_crc))

img = SxfsImageBuilder() \
    .add_file(SxfsFile(sys.argv[1], 0xAA)) \
    .add_file(SxfsFile(sys.argv[2], 0xBB))
update_file = open(sys.argv[3], "wb")
update_file.write(img.serialize())

update_file.close()
app_file.close()
hdr_file.close()