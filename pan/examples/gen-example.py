import struct
import math

def gen_add():
    msg = b''

    msg += b'test\x00\x00\x00\x00' # pref = 'test'
    msg += b'add\x00\x00\x00\x00\x00' # name = 'add'
    msg += (1).to_bytes(4, 'little') # seqno
    msg += (16).to_bytes(2, 'little') # length
    msg += (0).to_bytes(2, 'little') # flags

    msg += (1).to_bytes(8, 'little') # argument 1
    msg += (2).to_bytes(8, 'little') # argument 2

    return msg

def gen_everything():

    msg = b''
    
    msg += (1).to_bytes(4, 'little') # id
    msg += b'hellorld' # char64
    msg += (42).to_bytes(1, 'little') # int8
    msg += (1234).to_bytes(2, 'little') # int16
    msg += (123456789).to_bytes(4, 'little') # int32
    msg += (123456789012345678).to_bytes(8, 'little') # int64
    msg += struct.pack('f', math.pi) # float
    msg += struct.pack('d', math.e) # double

    hw = 'Hello world!'
    msg += len(hw).to_bytes(2, 'little') + hw.encode()

    blob = b'\x00\xff\x00\x88'
    msg += len(blob).to_bytes(2, 'little') + blob

    msg += b'\01' # bool

    head = b''

    head += b'test\x00\x00\x00\x00'
    head += b'every\x00\x00\x00'
    head += (1).to_bytes(4, 'little')
    head += len(msg).to_bytes(2, 'little')
    head += (0).to_bytes(2, 'little')
    head += msg

    return head

with open('add.bmsg', 'wb') as f:
    f.write(gen_add())

with open('every.bmsg', 'wb') as f:
    f.write(gen_everything())

