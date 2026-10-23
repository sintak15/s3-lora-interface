"""Exercise the real bundled map viewer with all external network access blocked."""
import json
from pathlib import Path
import struct

ROOT = Path(__file__).resolve().parents[1]
HTML = (ROOT / 'src/WebUi.h').read_text(encoding='utf-8').split('R"HTML(')[1].split(')HTML"')[0]
PIXELS = b''.join(struct.pack('<H', (0xf800 if x < 128 else 0x07e0) if y < 128 else (0x001f if x < 128 else 0xffff)) for y in range(256) for x in range(256))
STATUS = dict(title='Offline map test', myNode='!00000001', myNodeName='Field node',
              wifiMode='AP', ip='192.168.4.1', gpsValid=False, heltecConfig={}, channels=[],
              nodes=[], battery=80, voltage=4.0, txHistory=[], mapTileStorageGeneration=3,
              offlineMapCenter=dict(lat=44.9537, lon=-93.09))

def main():
    from playwright.sync_api import sync_playwright

    with sync_playwright() as p:
        browser = p.chromium.launch(headless=True)
        for width in (390, 1280):
            page = browser.new_page(viewport=dict(width=width, height=900))
            errors, external, tile_requests = [], [], []
            state = dict(mode='ready', generation=3)
            page.on('pageerror', lambda error: errors.append(str(error)))

            def route(request):
                url = request.request.url
                if not url.startswith('http://mesh.test/'):
                    external.append(url); request.abort(); return
                route_path = url.split('http://mesh.test/')[1]
                if not route_path:
                    request.fulfill(content_type='text/html', body=HTML)
                elif route_path.startswith('assets/'):
                    name = 'leaflet.js' if route_path.endswith('.js') else 'leaflet.css'
                    request.fulfill(content_type='application/javascript' if name.endswith('.js') else 'text/css', body=(ROOT / 'src/vendor/leaflet' / name).read_bytes())
                elif route_path == 'status':
                    request.fulfill(content_type='application/json', body=json.dumps(dict(STATUS, mapTileStorageGeneration=state['generation'])))
                elif route_path.startswith('map/tile?'):
                    tile_requests.append(url)
                    mode = state['mode']
                    if mode in ('missing', 'unreadable', 'busy'):
                        request.fulfill(status=dict(missing=404, unreadable=422, busy=503)[mode], body='unavailable'); return
                    zoom = '12' if mode == 'coarse' else '14'
                    request.fulfill(content_type='application/octet-stream', headers={'X-Tile-Zoom': zoom, 'X-Map-Generation': str(state['generation'])}, body=PIXELS if mode != 'truncated' else PIXELS[:20])
                else:
                    request.abort()

            page.route('**/*', route)
            page.goto('http://mesh.test/')
            page.locator('[data-page="map"]').click()
            page.wait_for_function("document.querySelector('#realMap canvas[data-state=ready]')")
            assert page.evaluate('realMapReady && offlineCentered')
            assert page.evaluate('leafletMap.getZoom()') == 14
            assert page.locator('.leaflet-control-scale-line').count() == 1
            assert 'm' not in page.locator('.leaflet-control-scale-line').inner_text().replace('mi', '')
            assert page.evaluate('document.documentElement.scrollWidth <= innerWidth')
            assert page.locator('#realMap canvas').first.evaluate("tile=>getComputedStyle(tile).borderRadius") == '0px'
            # RGB565 little-endian expansion and row orientation must match SD tiles.
            colors = page.evaluate("""() => { const t=document.createElement('canvas');t.width=t.height=256;const bytes=new Uint8Array(131072);for(let i=0;i<65536;i++){bytes[i*2]=0;bytes[i*2+1]=248;}paintOfflineTile(t,{x:5,y:6,z:14},bytes.buffer,14);return [...t.getContext('2d').getImageData(20,20,1,1).data]; }""")
            assert colors == [255, 0, 0, 255], colors
            cropped = page.evaluate("""() => {const bytes=new Uint8Array(131072);for(let y=0;y<256;y++)for(let x=0;x<256;x++){const c=y>=128?(x<128?31:65535):(x<128?63488:2016),i=(y*256+x)*2;bytes[i]=c&255;bytes[i+1]=c>>8;}const t=document.createElement('canvas');t.width=t.height=256;paintOfflineTile(t,{x:5,y:6,z:14},bytes.buffer,12);return [...t.getContext('2d').getImageData(20,20,1,1).data];}""")
            assert cropped == [0, 0, 255, 255], cropped
            page.evaluate('leafletMap.panBy([80,40],{animate:false})')
            center = page.evaluate('JSON.stringify(leafletMap.getCenter())')
            page.evaluate('refresh()')
            assert page.evaluate('JSON.stringify(leafletMap.getCenter())') == center, 'Polling reset the pan'
            state['mode'] = 'coarse'
            page.evaluate('reloadOfflineMap()')
            page.wait_for_function("document.querySelector('#realMap canvas[data-state=coarse]')")
            assert page.locator('#mapMeta').inner_text().find('coarser') >= 0
            page.screenshot(path=str(ROOT / f'build/web-offline-map-{width}.png'), full_page=True)
            state['mode'] = 'missing'
            page.evaluate('reloadOfflineMap()')
            page.wait_for_function("document.querySelector('#realMap canvas[data-state=missing]')")
            assert 'missing' in page.locator('#mapMeta').inner_text()
            for mode in ('unreadable', 'truncated', 'busy'):
                state['mode'] = mode
                page.evaluate('reloadOfflineMap()')
                page.wait_for_function("document.querySelector('#realMap canvas[data-state=error]')", timeout=15000)
            state['mode'] = 'ready'; state['generation'] = 4
            page.evaluate('refresh()')
            page.wait_for_function("document.querySelector('#realMap canvas[data-state=ready]')")
            assert page.evaluate('offlineGeneration') == 4
            page.locator('button:has-text("My location")').click()
            lat = page.evaluate('leafletMap.getCenter().lat')
            # Leaflet rounds the projected center to a screen pixel.
            assert abs(lat - STATUS['offlineMapCenter']['lat']) < .0001, lat
            assert not external, external
            assert not errors, errors
            assert tile_requests
            page.close()
            print(f'Offline map browser checks passed at {width}px: local assets, tiles, RGB565, fallback crop, pan, errors, recovery, imperial scale.')
        browser.close()

if __name__ == '__main__':
    main()
