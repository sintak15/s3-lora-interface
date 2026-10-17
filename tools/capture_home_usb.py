"""Capture the verification firmware's Home screen as a PNG over USB."""
import argparse
import base64
from pathlib import Path
import time
import zlib
import serial
from PIL import Image

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port',required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    port=serial.Serial()
    port.port=args.port;port.baudrate=115200;port.timeout=.2;port.write_timeout=5
    port.dtr=False;port.rts=False;port.open()
    pending=bytearray()
    def command(text):
        nonlocal pending
        port.write((text+'\n').encode())
        end=time.monotonic()+10
        while time.monotonic()<end:
            if b'\n' not in pending:
                pending.extend(port.read(max(1,port.in_waiting)))
                continue
            raw,pending=pending.split(b'\n',1)
            line=raw.decode('ascii',errors='replace').strip()
            if line.startswith('[SCREEN] ERROR'):raise RuntimeError(line)
            if line.startswith('[SCREEN] '):return line[len('[SCREEN] '):]
        raise TimeoutError(text)
    try:
        ready=command('screen-home').split()
        assert ready[0]=='READY',ready
        width,height,size=map(int,ready[1:]);assert size==width*height*2
        pixels=bytearray()
        while len(pixels)<size:
            for attempt in range(5):
                reply=command(f'screen-read {len(pixels)}').split()
                try:
                    assert reply[0]=='DATA' and int(reply[1])==len(pixels),reply[:3]
                    chunk=base64.b64decode(reply[3],validate=True)
                    assert f'{zlib.crc32(chunk):08x}'==reply[2], 'Capture chunk CRC mismatch'
                    break
                except (ValueError,AssertionError,IndexError):
                    if attempt==4:raise
            pixels.extend(chunk)
        assert len(pixels)==size
        colors=bytearray(width*height*3)
        for i in range(width*height):
            value=pixels[i*2]|pixels[i*2+1]<<8
            r,g,b=value>>11,(value>>5)&63,value&31
            colors[i*3:i*3+3]=bytes(((r<<3)|(r>>2),(g<<2)|(g>>4),(b<<3)|(b>>2)))
        args.output.parent.mkdir(parents=True,exist_ok=True)
        Image.frombytes('RGB',(width,height),bytes(colors)).save(args.output)
        print(f'Captured {width}x{height} Home screen to {args.output}')
    finally:
        try:command('screen-close')
        finally:port.close()

if __name__=='__main__':main()
