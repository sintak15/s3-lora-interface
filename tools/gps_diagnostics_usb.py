"""Observe receiver UART health over USB without exporting GPS coordinates."""
import argparse
import json
from pathlib import Path
import time
import serial

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port',required=True)
    parser.add_argument('--duration',type=float,default=30)
    parser.add_argument('--interval',type=float,default=5)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    port=serial.Serial();port.port=args.port;port.baudrate=115200;port.timeout=.2;port.write_timeout=5
    port.dtr=False;port.rts=False;port.open()
    pending=bytearray();samples=[]
    try:
        end=time.monotonic()+args.duration;next_sample=0
        while time.monotonic()<end:
            now=time.monotonic()
            if now>=next_sample:
                port.write(b'gps\n');next_sample=now+max(.5,args.interval)
            pending.extend(port.read(max(1,port.in_waiting)))
            while b'\n' in pending:
                raw,pending=pending.split(b'\n',1)
                line=raw.decode(errors='replace').strip()
                if not line.startswith('[GPS] {'):continue
                sample=json.loads(line[len('[GPS] '):]);sample['observedAt']=time.time();samples.append(sample)
                print(json.dumps(sample),flush=True)
        assert len(samples)>=2,'No GPS diagnostic samples received; firmware v0.1.90+ is required'
        first,last=samples[0],samples[-1]
        summary={key:last[key]-first[key] for key in ('localGpsBytes','localGpsValidSentences','localGpsFailedChecksum','localGpsSentences')}
        summary['seconds']=round(last['observedAt']-first['observedAt'],1)
        args.output.parent.mkdir(parents=True,exist_ok=True)
        args.output.write_text(json.dumps({'samples':samples,'delta':summary},indent=2),encoding='utf-8')
        print('DELTA',json.dumps(summary),flush=True)
    finally:
        port.close()

if __name__=='__main__':main()
