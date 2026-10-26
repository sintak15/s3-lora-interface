"""Inspect real LVGL bounds and TFT frames on verification firmware over Wi-Fi."""
import argparse
import base64
import json
import time
import urllib.error
import urllib.parse
import urllib.request
import zlib
from pathlib import Path
from PIL import Image

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--base-url', required=True)
parser.add_argument('--auth-file', required=True, type=Path)
parser.add_argument('--output', type=Path, default=Path('build/display-layout'))
parser.add_argument('--screen')
parser.add_argument('--font', choices=['normal', 'readable'])
parser.add_argument('--case', default='real')
args = parser.parse_args()
folder = args.output; folder.mkdir(parents=True, exist_ok=True)
creds = json.loads(args.auth_file.read_text(encoding='utf-8'))
auth = 'Basic ' + base64.b64encode((creds['username'] + ':' + creds['password']).encode()).decode()

def request(path, fields=None):
    data = urllib.parse.urlencode(fields).encode() if fields is not None else None
    headers = {'Authorization': auth}
    if data is not None: headers['Content-Type'] = 'application/x-www-form-urlencoded'
    for attempt in range(8):
        try:
            with urllib.request.urlopen(urllib.request.Request(args.base_url.rstrip('/') + path, data=data, headers=headers), timeout=25) as r:
                return r.read()
        except urllib.error.HTTPError as e:
            if e.code != 409 or attempt == 7: raise
            time.sleep(1)

def inspect(screen, font, case='real', capture=False):
    result = json.loads(request('/diagnostics/ui-layout', dict(screen=screen, font=font, case=case, capture=int(capture))))
    name = f'{font}-{screen}-{case}'
    (folder / f'{name}.json').write_text(json.dumps(result, indent=2), encoding='utf-8')
    if capture:
        raw = request('/diagnostics/ui-frame')
        assert len(raw) == 240 * 320 * 2 and zlib.crc32(raw) == result['frameCrc']
        pixels = bytearray(240 * 320 * 3)
        for i in range(240 * 320):
            value = raw[i*2] | raw[i*2+1] << 8
            r, g, b = value >> 11, (value >> 5) & 63, value & 31
            pixels[i*3:i*3+3] = bytes(((r << 3) | (r >> 2), (g << 2) | (g >> 4), (b << 3) | (b >> 2)))
        Image.frombytes('RGB', (240,320), bytes(pixels)).save(folder / f'{name}.png')
    print(json.dumps(dict(screen=screen, font=font, case=case, labels=result['labels'], issues=result['issueCount'])), flush=True)
    return result

if args.screen:
    result = inspect(args.screen, args.font or 'normal', args.case, True)
    print(json.dumps(result['issues'], indent=2), flush=True)
    raise SystemExit(bool(result['issueCount']))
else:
    screens = ['home','menu','messages','health','nodes','node-detail','mesh-health','packets','delivery',
               'public','secondary','direct','message-test','map','coverage','system','interface','serial',
               'radio','gps','gps-config','gps-advanced','wifi','wifi-stats','wifi-connect','wifi-scan',
               'display','sound','battery','power-logs','games','pets','pet','pet-care','gameboy','gameboy-player','conversations']
    results = []
    for font in (['normal','readable'] if not args.font else [args.font]):
        for screen in screens:
            results.append(inspect(screen, font, 'real', screen in ['home','menu','battery','sound','gps-advanced']))
        for case in ['runtime','charging','critical','unknown','missing','long']:
            results.append(inspect('home', font, case, True))
        results.append(inspect('nodes', font, 'detail', True))
    (folder / 'native-layout-results.json').write_text(json.dumps(results, indent=2), encoding='utf-8')
    print('Total checks:',len(results),'checks with issues:',sum(bool(r['issueCount']) for r in results),flush=True)

    raise SystemExit(any(r['issueCount'] for r in results))
