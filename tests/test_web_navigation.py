"""Exercise settings navigation and edit preservation in a real offline browser."""
import json
from pathlib import Path
from urllib.parse import parse_qs

ROOT = Path(__file__).resolve().parents[1]
HTML = (ROOT / 'src/WebUi.h').read_text(encoding='utf-8').split('R"HTML(')[1].split(')HTML"')[0]
STATUS = dict(
    title='Field node', deviceName='Field node', hostname='field-node', webUser='admin',
    apSsid='Field node', apChannel=6, myNode='!00000001', myNodeName='Field node',
    wifiMode='Local', wifiEnabled=True, ip='192.168.1.100', linkHealth='responsive',
    lastRadioResponseSec=3, fuelGaugePresent=True, battery=78, voltage=3.89,
    batteryRuntimeMinutes=245, batteryRuntimeBasis='current', powerState='Battery',
    gpsValid=False, localGpsFreshFix=False, sdAvailable=True, sdStatus='logging',
    nodes=[], txHistory=[], channels=[dict(index=0, name='Primary', role='PRIMARY', enabled=True, pskSize=16)],
    heltecConfig=dict(ageSec=3, moduleAgeSec=3, network=dict(valid=True, wifi=True, ssid='Original network',
        ntp='pool.ntp.org', syslog='', eth=False, ipv6=False, udp=False, pskSet=True),
        modules=dict(mqtt=dict(valid=True, enabled=False, address='mqtt.example', root='msh', username='',
            encryption=True, json=False, tls=True, proxy=False, mapReporting=False, passwordSet=True))),
)


def main():
    from playwright.sync_api import sync_playwright
    folder = ROOT / 'build/webui-review'
    folder.mkdir(exist_ok=True)
    with sync_playwright() as p:
        browser = p.chromium.launch(headless=True)
        for width in (390, 1280):
            page = browser.new_page(viewport=dict(width=width, height=900))
            errors, posts, external = [], [], []
            state = dict(setup=False, reject_power=False, drop_power=False)
            page.on('pageerror', lambda error: errors.append(str(error)))

            def route(request):
                url = request.request.url
                if not url.startswith('http://mesh.test/'):
                    external.append(url)
                    request.abort()
                    return
                resource = url.removeprefix('http://mesh.test/')
                if request.request.method == 'POST':
                    posts.append((resource, parse_qs(request.request.post_data or '', keep_blank_values=True)))
                    if resource == 'heltec/power' and state['drop_power']:
                        request.abort()
                    elif resource == 'heltec/power' and state['reject_power']:
                        request.fulfill(status=400, content_type='text/plain', body='Invalid sleep interval')
                    else:
                        request.fulfill(status=200, content_type='text/plain', body='Saved')
                elif not resource:
                    request.fulfill(content_type='text/html', body=HTML)
                elif resource == 'status':
                    request.fulfill(content_type='application/json', body=json.dumps(dict(STATUS, setupRequired=state['setup'])))
                elif resource.startswith('assets/'):
                    name = 'leaflet.js' if resource.endswith('.js') else 'leaflet.css'
                    request.fulfill(content_type='text/javascript' if name.endswith('.js') else 'text/css',
                                    body=(ROOT / 'src/vendor/leaflet' / name).read_bytes())
                else:
                    request.abort()

            page.route('**/*', route)
            page.goto('http://mesh.test/')
            page.wait_for_selector('#stats .stat')
            assert '▾' in page.locator('#toolsLabel').evaluate("e=>getComputedStyle(e,'::after').content")
            page.locator('#tabs>[data-page="config"]').click()
            assert page.locator('[data-settings-group]').count() == 5
            assert page.locator('.settingsPanel:visible').count() == 0
            assert page.locator('#config input:visible').count() == 1, 'Settings opens with search, not a wall of controls'
            assert page.evaluate('document.documentElement.scrollWidth <= innerWidth')
            assert page.evaluate('document.documentElement.scrollHeight') < 1500
            page.screenshot(path=str(folder / f'after-settings-home-{width}.png'), full_page=True)

            page.locator('[data-settings-group="connections"]').click()
            assert page.locator('[data-settings-panel]').count() == 4
            assert page.locator('#settingsBackCategory').inner_text() == 'Connections'
            assert page.locator('#settingsBackCategory').is_visible()
            page.locator('[data-settings-panel="cfgNetwork"]').click()
            assert page.locator('.settingsPanel:visible').count() == 1
            page.locator('#hcNetSsid').fill('Edited network')
            page.locator('#hcNetPsk').fill('not-a-real-password')
            page.locator('#settingsBackRoot').click()
            page.evaluate('refresh()')
            page.locator('[data-settings-group="connections"]').click()
            page.locator('[data-settings-panel="cfgNetwork"]').click()
            assert page.locator('#hcNetSsid').input_value() == 'Edited network'
            assert page.locator('#hcNetPsk').input_value() == 'not-a-real-password', 'Polling must retain a pending password after navigation'
            page.locator('#settingsBackRoot').click()
            page.locator('#settingsSearch').fill('mqtt')
            page.locator('[data-settings-panel="cfgMqtt"]').click()
            page.locator('#hcMqttPass').fill('another-pending-secret')
            page.locator('#settingsBackRoot').click()
            page.evaluate('refresh()')
            page.locator('#settingsSearch').fill('mqtt')
            page.locator('[data-settings-panel="cfgMqtt"]').click()
            assert page.locator('#hcMqttPass').input_value() == 'another-pending-secret'

            page.locator('#settingsBackRoot').click()
            page.locator('#settingsSearch').fill('gps attempt')
            assert page.locator('[data-settings-panel]').count() == 1
            page.locator('[data-settings-panel="cfgPosition"]').click()
            assert page.locator('#hcGpsAttempt').is_visible(), 'Search must reveal matching advanced fields'
            page.screenshot(path=str(folder / f'after-settings-editor-{width}.png'), full_page=True)

            # Every settings editor must be reachable, with all controls still available.
            ids = page.evaluate('settingsCatalog.map(item=>item.id)')
            assert len(ids) == len(set(ids))
            for ident in ids:
                page.evaluate('(id)=>openSettingsPanel(id)', ident)
                assert page.locator('.settingsPanel:visible').count() == 1
                panel = page.locator('#' + ident)
                panel.locator('details').evaluate_all('items=>items.forEach(item=>item.open=true)')
                assert panel.locator('input,select').count() == panel.locator('input:visible,select:visible').count(), ident
                assert panel.locator('button').count() > 0, ident

            page.evaluate("openSettingsPanel('cfgNeighbor')")
            page.locator('#hcNeighborEnabled').check()
            page.locator('#hcNeighborSecs').fill('600')
            page.locator('#cfgNeighbor button').click()
            page.wait_for_timeout(50)
            module_posts = [post for post in posts if post[0] == 'heltec/module']
            assert len(module_posts) == 1, posts
            assert module_posts[0][1]['section'] == ['neighbor_info']
            assert module_posts[0][1]['interval'] == ['600']
            assert page.locator('#moduleStatus').is_visible()
            page.evaluate("openSettingsPanel('cfgPower')")
            state['reject_power'] = True
            page.locator('#cfgPower>div.formgrid>button').click()
            page.wait_for_function("moduleStatus.textContent.includes('Invalid sleep interval')")
            assert page.locator('#moduleStatus').is_visible(), 'Non-module editors need visible apply feedback too'
            state['reject_power'] = False
            state['drop_power'] = True
            page.locator('#cfgPower>div.formgrid>button').click()
            page.wait_for_function("moduleStatus.textContent.includes('result is unknown')")
            state['drop_power'] = False

            page.evaluate('openChannelEditor(0)')
            page.wait_for_selector('#cfgChannel:visible')
            assert page.locator('#hcChanIndex').input_value() == '0'
            page.locator('#toolsLabel').click()
            page.keyboard.press('Escape')
            assert not page.locator('#navTools').evaluate('element=>element.open')
            page.locator('#toolsLabel').click()
            page.locator('#tabs [data-page="diagnostics"]').click()
            assert page.locator('#diagStats .stat').count() == 8
            assert '78% · 3.89 V' in page.locator('#diagStats').inner_text()
            assert 'Â' not in page.locator('#diagnostics').inner_text()
            assert page.locator('#diagBatteryStats').is_visible() is False
            page.screenshot(path=str(folder / f'after-health-{width}.png'), full_page=True)
            page.locator('summary', has_text='Battery & runtime').click()
            assert page.locator('#diagBatteryStats .stat').count() == 6
            colors = page.locator('#diagBatteryStats .stat').first.evaluate("e=>({label:getComputedStyle(e.querySelector('.label')).color,value:getComputedStyle(e.querySelector('.value')).color})")
            assert colors == dict(label='rgb(133, 191, 255)', value='rgb(244, 248, 255)'), colors
            assert page.evaluate('document.documentElement.scrollWidth <= innerWidth')

            state['setup'] = True
            page.evaluate('refresh()')
            page.wait_for_selector('body.setup-required')
            assert page.locator('#cfgInterface').is_visible()
            assert page.locator('#settingsDirectory').is_visible() is False
            assert page.locator('#settingsSearch').is_visible() is False
            assert page.locator('#tabs button:visible').count() == 1
            assert page.locator('#tabs button:visible').get_attribute('data-page') == 'config'
            assert not errors, errors
            assert not external, external
            page.close()
            print(f'Web navigation passed at {width}px: focused settings, search, advanced controls, retained edits/secrets, module save, channel shortcut, health groups, blue labels and setup lock.')
        browser.close()


if __name__ == '__main__':
    main()
