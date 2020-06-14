#!/usr/bin/env python2

"""
convert a gimp .h output file to binary, compress w/zlib then output
the compressed binary.
Also, compresses the color data to 4 bits/color, selecting the
high order bits. 

Output is suitable for input to bin2h.py for conversion to a C array.
"""

import sys, os, string, zlib

def eprintf(fmt, *args):
    if args:
        fmt = fmt % args
    sys.stderr.write('%s' % (fmt,))

if len(sys.argv) > 1:
    infile = sys.argv[1]
    f = open(infile)
else:
    f = sys.stdin

w = -1
h = -1
depth_in_bytes = 3

ST_INIT = 1
ST_DIMS = 2

state = ST_INIT

def dprintf(fmt, *args):
    if args:
        fmt = fmt % args
    sys.stderr.write(fmt)

line_buf = []
line_buf_idx = 0
def getword(f):
    global line_buf
    global line_buf_idx

    while line_buf == []:
        dprintf("filling line\n")
        line = f.readline()
        if line[0] == '#':
            dprintf('discard comment>%s<\n', line)
            continue
        if not line:
            return line
        line_buf = string.split(line)

    dprintf("buf>%s<\n", line_buf)
    ret = line_buf.pop(0)
    dprintf("ret>%s<\n", ret)
    return (ret)
    
while 1:
    tok = getword(f)
    dprintf('tok>%s<\n', tok)
    
    if not tok:
        eprintf('Cannot find start of data.\n')
        sys.exit(2)

    if state == ST_INIT:
        if tok == 'P3' or tok == 'P6':
            if tok == 'P3':
                type = 'ascii'
            else:
                type = 'binary'
            state = ST_DIMS
        else:
            eprintf("Bad format in pnm file, tok>%s<.\n" % tok)
            sys.exit(2)

    elif state == ST_DIMS:
        w = eval(tok)
        h = eval(getword(f))
        if w != 320 or h != 240:
            eprintf("Unsupported dimensions in pnm file.\n")
            eprintf('w: %d, h: %d\n', w, h)
            sys.exit(2)
        getword(f)                # read & discard the color depth
        break


def from_ascii(f):
    i = 0
    odata = []
    t = ''
    while 1:
        line = f.readline()
        if not line or line == '\n':
            break
        for n in string.split(line):
            t = t + chr(string.atoi(n))

        if len(t) > 256:
            odata.append(t)
            t = ''

        i = i + 4
        if i % 1000 == 0:
            eprintf('<')

    if t:
        odata.append(t)
        
    return string.join(odata, '')

def from_bin(f):
    eprintf('<')
    num = w * h * depth_in_bytes
    s = f.read(num)
    eprintf('<')
    return s

if type == 'binary':
    data = from_bin(f)
else:
    #eprintf('ascii not supported yet.\n')
    #sys.exit(1)
    data = from_ascii(f)
    
i = 0
data_len = len(data)
odata = []
t = ''
while i < data_len:
    # these are 8 bit values
    r = ord(data[i])
    g = ord(data[i+1])
    b = ord(data[i+2])

    # the 3800 supports full 16 bit color so we should keep
    # it here even though the 3600 will ignore the low order
    # bits
    # old value bpix = ((r & 0xf0)<<4) | ((g & 0xf0)) | ((b & 0xf0)>>4)
    # 16 bit color is 5 reds, 6 greens, 5 blues
    bpix = ((r & 0xf8)<< 8 ) 
    bpix = bpix | ((g & 0xfc)<< 3)
    bpix = bpix | ((b & 0xf8) >> 3)

    #print '0x%x 0x%x 0x%x==>%x' % (r, g, b, bpix)

    # little endian
    t = t + chr(bpix & 0x00ff) + chr((bpix>>8) & 0x00ff)
    
    # t = t + chr((bpix>>8) & 0x00ff) + chr(bpix & 0x00ff)
    if len(t) > 256:
        odata.append(t)
        t = ''

    i = i + 3
    if i % 1000 == 0:
        eprintf('>')

if t:
    odata.append(t)
data = string.join(odata, '')
    
zdata = zlib.compress(data, 9)

#of = open('bz', 'w')
#of.write(zdata)
#of.close()

sys.stdout.write(zdata)
eprintf('\nlen of zdata: %s\n', len(zdata))

sys.exit(0)

