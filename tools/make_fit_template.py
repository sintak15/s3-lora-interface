"""Generate 1:1 paper fit templates from the V4 OpenSCAD parameters."""
from pathlib import Path
import re
import html

ROOT = Path(__file__).resolve().parents[1]
source = (ROOT / 'enclosure_v4.scad').read_text()

def value(name):
    raw = re.search(r'^' + re.escape(name) + r'\s*=\s*([^;]+);', source, re.M)[1]
    if raw.startswith('['):
        return [float(v) for v in raw.strip('[]').split(',')]
    return float(raw)

out = ROOT / 'docs' / 'fit-template'
out.mkdir(exist_ok=True)
w, length = value('case_w'), value('case_l')

def page(rear=False):
    items = []
    def rect(x, y, a, b, label='', dashed=False, radius=0):
        # Both sheets show the interior with antenna end at the top.
        # The rear interior is mirrored from assembly coordinates.
        if rear:
            x = w-x-a
        yy = length-y-b
        dash = ' stroke-dasharray="1.5 1"' if dashed else ''
        items.append(f'<rect x="{x}" y="{yy}" width="{a}" height="{b}" rx="{radius}" fill="none" stroke="#111" stroke-width="0.25"{dash}/>')
        if label:
            items.append(f'<text x="{x+a/2}" y="{yy+b/2}" text-anchor="middle" font-size="2.5">{html.escape(label)}</text>')
    def hole(x,y,d):
        if rear: x=w-x
        y=length-y
        items.append(f'<circle cx="{x}" cy="{y}" r="{d/2}" fill="none" stroke="#111" stroke-width="0.2"/><path d="M{x-2},{y}h4 M{x},{y-2}v4" stroke="#111" stroke-width="0.15"/>')
    rect(0,0,w,length,radius=value('corner_r'))
    wall=value('wall')
    rect(wall,wall,w-2*wall,length-2*wall,dashed=True,radius=value('corner_r')-wall)
    for x in (5,w-5):
        for y in (5,length-5): hole(x,y,value('case_clearance_d'))
    if not rear:
        x,y=value('cyd_x'),value('cyd_y')
        rect(x,y,value('cyd_w'),value('cyd_l'),'CYD 50 × 86')
        rect(x,y+value('cyd_glass_y'),value('cyd_glass_w'),value('cyd_glass_l'),dashed=True)
        for dx in (0,value('cyd_hole_pitch_x')):
            for dy in (0,value('cyd_hole_pitch_y')):
                hole(x+value('cyd_hole_x')+dx,y+value('cyd_hole_y')+dy,value('cyd_mount_pilot_d'))
        rect(value('heltec_x'),value('heltec_y'),value('heltec_w'),value('heltec_l'),'Heltec')
        rect(value('heltec_x')+(value('heltec_w')-value('heltec_window_w'))/2,value('heltec_y')+value('heltec_window_y'),value('heltec_window_w'),value('heltec_window_l'),dashed=True)
        xy=value('gps_xy'); s=value('gps_size')
        rect(*xy,s[1],s[0],'GPS board Z=18')
        xy=value('patch_xy'); s=value('patch_size')
        rect(*xy,s[0],s[1],'Patch below GPS',True)
    else:
        for pos,size,label in [('power_xy','power_size','Power 30 × 70'),('battery_xy','battery_size','Holder 22.6 × 76.4'),('gauge_xy','gauge_size','MAX'),('speaker_xy','speaker_size','Speaker')]:
            xy=value(pos); s=value(size)
            rect(*xy,s[0],s[1],label)
        # Charging opening's extent along the left assembly wall.
        xy=value('power_xy'); s=value('power_size')
        cy=xy[1]+s[1]-value('power_usb_y_from_top')
        rect(0,cy-value('power_usb_w')/2,wall,value('power_usb_w'),dashed=True)
    title='REAR — looking into rear shell' if rear else 'FRONT — looking into front shell'
    return f'''<svg xmlns="http://www.w3.org/2000/svg" width="190mm" height="250mm" viewBox="0 0 190 250">
<g font-family="Arial, sans-serif" fill="#111">
<text x="8" y="12" font-size="5">V4 enclosure: full-size fit template</text>
<text x="8" y="21" font-size="3.5">{title}</text>
<text x="8" y="29" font-size="3">ANTENNA / SWITCH END ↑ · All dimensions in mm</text>
<g transform="translate(35 38)">{''.join(items)}</g>
<text x="8" y="175" font-size="3">Solid: component envelope / shell. Dashed: wall / glass / overlay.</text>
<text x="8" y="182" font-size="3">Place actual parts on their outlines; check edges and hole centres.</text>
<text x="8" y="189" font-size="3">GPS board overlaps its patch at different heights.</text>
<text x="8" y="196" font-size="3">Paper checks XY only; verify height, wires, ports and mounts in CAD/coupon.</text>
<text x="8" y="203" font-size="3">Print at 100% / Actual size. Disable Fit to page. Measure ruler first.</text>
<path d="M10 219h100 M10 216v6 M60 216v6 M110 216v6" fill="none" stroke="#111" stroke-width="0.3"/>
<text x="10" y="228" font-size="3">0</text><text x="57" y="228" font-size="3">50</text><text x="105" y="228" font-size="3">100 mm</text>
<rect x="145" y="213" width="25" height="25" fill="none" stroke="#111" stroke-width="0.25"/>
<text x="144" y="244" font-size="3">25 × 25 mm check</text>
</g></svg>'''

for name,rear in [('front',False),('rear',True)]:
    (out/f'{name}.svg').write_text(page(rear),encoding='utf-8')
(out/'print.html').write_text('''<!doctype html><meta charset="utf-8"><title>V4 full-size fit template</title>
<style>@page{size:letter portrait;margin:10mm}body{margin:0}img{display:block;width:190mm;height:250mm;break-after:page}img:last-child{break-after:auto}</style>
<img src="front.svg" alt="Front interior fit template"><img src="rear.svg" alt="Rear interior fit template">
''',encoding='utf-8')
print(f'Created front.svg, rear.svg and print.html in {out}')
