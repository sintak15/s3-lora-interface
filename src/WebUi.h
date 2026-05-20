#pragma once

#include <Arduino.h>

static const char WEB_UI_HTML[] PROGMEM = R"HTML(
<!doctype html><html><head><meta name="viewport" content="width=device-width,initial-scale=1">
<title>LoRa Interface</title>
<link rel="stylesheet" href="https://unpkg.com/leaflet@1.9.4/dist/leaflet.css">
<style>
*{box-sizing:border-box}html,body{min-height:100%;background:#050807;color:#f4fff9}
body{margin:0;background:#050807!important;color:#f4fff9!important;font-family:"Comic Sans MS","Comic Sans","Trebuchet MS",system-ui,sans-serif}
header{position:sticky;top:0;z-index:5;padding:8px 10px;background:#080d0b;border-bottom:1px solid #24483e}
.top{display:flex;align-items:center;justify-content:space-between;gap:8px}h1{font-size:16px;margin:0;color:#f4fff9}#ip{color:#8ab7a6;font-size:12px}
nav{display:flex;gap:6px;overflow-x:auto;padding-top:8px;scrollbar-width:none}nav button{flex:0 0 auto;margin:0;padding:8px 10px;background:#101816;color:#d9fff0;border:1px solid #2f705f}
nav button.active{background:#00c985;color:#001b12;border-color:#00c985}main{padding:8px;max-width:980px;margin:0 auto}
section.page{display:none}.page.active{display:block}.card{background:#101816;color:#f4fff9;border:1px solid #24483e;border-radius:6px;padding:10px;margin-bottom:8px}
h2{font-size:13px;line-height:1.1;margin:0 0 8px;color:#68ffc0;text-transform:uppercase}.stats{display:grid;grid-template-columns:1fr 1fr;gap:6px}
.stat{background:#07100d;color:#f4fff9;padding:7px;border-radius:4px}.label{color:#8ab7a6;font-size:11px}.value{font-size:15px;color:#f4fff9;word-break:break-word}
pre{white-space:pre-wrap;overflow:auto;max-height:62vh;margin:0;color:#e8fff5;background:#07100d;border-radius:6px;padding:8px;font:12px ui-monospace,Consolas,monospace}
input,select{box-sizing:border-box;width:100%;padding:11px;background:#07100d!important;border:1px solid #2f705f;color:#fff!important;border-radius:6px}
button{margin-top:6px;padding:10px 12px;border:0;border-radius:6px;background:#00c985;color:#001b12;font-weight:700}
.actions{display:flex;gap:6px;flex-wrap:wrap}.actions button{flex:1 1 130px}.formgrid{display:grid;grid-template-columns:1fr 1fr;gap:8px}
.field{display:flex;flex-direction:column;gap:4px}.field label{color:#8ab7a6;font-size:11px;text-transform:uppercase}.full{grid-column:1/-1}
.hint{color:#8ab7a6;font-size:12px;margin-top:8px}.statusbox{min-height:34px;background:#07100d;border:1px solid #24483e;border-radius:6px;padding:8px;color:#d9fff0}.checkrow{display:flex;align-items:center;gap:8px;color:#d9fff0;font-size:13px}.checkrow input{width:auto}.primaryAction{grid-column:1/-1}.dangerAction{background:#16332b;color:#d9fff0;border:1px solid #2f705f}
a{color:#68ffc0;text-decoration:none}.links{display:grid;grid-template-columns:1fr 1fr;gap:6px}.links a{padding:9px 8px;border:1px solid #2f705f;border-radius:6px;background:#07100d;text-align:center}
table{width:100%;border-collapse:collapse;font-size:12px;color:#f4fff9}td,th{border-bottom:1px solid #203b35;padding:6px 4px;text-align:left}
.tablewrap{overflow:auto}#realMap{width:100%;height:58vh;min-height:320px;background:#07100d;border:1px solid #24483e;border-radius:6px;box-sizing:border-box;overflow:hidden}
canvas{width:100%;height:320px;background:#07100d;border:1px solid #24483e;border-radius:6px;box-sizing:border-box}.hidden{display:none}.mapMeta{color:#8ab7a6;font-size:12px;margin-top:8px}
.leaflet-container{background:#07100d;color:#10231d}.leaflet-popup-content-wrapper,.leaflet-popup-tip{background:#101816;color:#e8fff5}
@media(max-width:560px){main{padding:6px}.stats,.formgrid{grid-template-columns:1fr}.full{grid-column:auto}.card{padding:8px}.links{grid-template-columns:1fr}#realMap{height:52vh}nav button{padding:8px 9px}}
</style></head><body><header><div class="top"><h1 id="appTitle">LoRa Interface</h1><div id="ip"></div></div>
<nav id="tabs"><button data-page="overview" class="active">Overview</button><button data-page="chat">Chat</button><button data-page="nodes">Nodes</button><button data-page="map">Map</button><button data-page="config">Config</button><button data-page="storage">Storage</button><button data-page="diagnostics">Diagnostics</button><button data-page="serialPage">Serial</button><button data-page="logs">Logs</button></nav></header><main>
<section class="page active" id="overview"><div class="card"><h2>Radio</h2><div class="stats" id="stats"></div><button onclick="fetch('/config',{method:'POST'})">Request Config</button></div><div class="card"><h2>Current Config</h2><div class="stats" id="configRef"></div><div class="tablewrap"><table><thead><tr><th>Index</th><th>Role</th><th>Name</th><th>State</th><th>Up</th><th>Down</th><th>Key</th></tr></thead><tbody id="channelRef"></tbody></table></div></div></section>
<section class="page" id="chat"><div class="card"><h2>Send Message</h2><div class="formgrid"><div class="field"><label for="chatChannel">Channel</label><select id="chatChannel"><option value="public">Public</option><option value="family">Family</option></select></div><div class="field"><label for="msg">Message</label><input id="msg" maxlength="233" placeholder="Message to mesh"></div><button class="primaryAction" onclick="send()">Send</button></div></div><div class="card"><h2>Public</h2><pre id="publicChatLog"></pre></div><div class="card"><h2>Family</h2><pre id="familyChatLog"></pre></div><div class="card"><h2>Direct</h2><pre id="directChatLog"></pre></div></section>
<section class="page" id="map"><div class="card"><h2>Map</h2><div id="realMap"></div><canvas id="mapFallback" class="hidden" width="640" height="360"></canvas><div class="mapMeta" id="mapMeta"></div></div></section>
<section class="page" id="nodes"><div class="card"><h2>Nodes</h2><div class="tablewrap"><table><thead><tr><th>Node</th><th>Name</th><th>SNR</th><th>Age</th><th>GPS</th></tr></thead><tbody id="nodesBody"></tbody></table></div></div></section>
<section class="page" id="storage"><div class="card"><h2>SD Storage</h2><div class="stats" id="storageStats"></div><div class="actions"><button onclick="mountSd()">Mount SD</button><button onclick="saveSnapshot()">Save Snapshot</button><button onclick="downloadSnapshot()">Download Live Snapshot</button></div><div class="links"><a href="/sd/events">Events</a><a href="/sd/public">Public Chat</a><a href="/sd/private">Private Chat</a><a href="/sd/positions">Positions CSV</a><a href="/sd/status-snapshot">Status Snapshot</a><a href="/sd/last-location">Last GPS</a><a href="/sd/mapcache">Map Cache</a></div><div class="hint statusbox" id="snapshotStatus">Snapshots include status, counters, node list, channel summary, logs, and GPS/SD health.</div></div></section>
<section class="page" id="config"><div class="card"><h2>Radio And Identity</h2><div class="formgrid">
<div class="field"><label for="hcRegion">Region</label><select id="hcRegion"><option>US</option><option>EU_868</option><option>CN</option><option>JP</option><option>ANZ</option><option>KR</option><option>TW</option><option>RU</option><option>IN</option><option>UNSET</option></select></div>
<div class="field"><label for="hcPreset">Radio preset</label><select id="hcPreset"><option>LONG_FAST</option><option>LONG_SLOW</option><option>MEDIUM_FAST</option><option>MEDIUM_SLOW</option><option>SHORT_FAST</option><option>SHORT_SLOW</option></select></div>
<div class="field"><label for="hcHop">Hop limit</label><input id="hcHop" type="number" min="0" max="7" value="3"></div>
<div class="field"><label for="hcTx">TX power</label><input id="hcTx" type="number" min="0" max="30" value="0"></div>
<div class="field"><label for="hcName">Node long name</label><input id="hcName" maxlength="39" placeholder="Optional"></div>
<div class="field"><label for="hcShortName">Short name</label><input id="hcShortName" maxlength="4" placeholder="4 chars"></div>
<div class="field full"><label for="hcTz">Timezone</label><input id="hcTz" value="CST6CDT,M3.2.0,M11.1.0"></div>
<button class="primaryAction" onclick="applyHeltecLora()">Apply Radio Settings</button>
<button onclick="applyHeltecName()">Update Name</button><button onclick="applyHeltecTime()">Update Timezone</button>
</div><div class="hint statusbox" id="hcStatus">Ready</div></div>
<div class="card"><h2>Serial Module</h2><div class="formgrid">
<label class="checkrow"><input id="hcSerialEnabled" type="checkbox" checked>Enabled</label>
<label class="checkrow"><input id="hcSerialEcho" type="checkbox">Echo</label>
<div class="field"><label for="hcSerialRxd">RX pin</label><input id="hcSerialRxd" type="number" min="0" max="48" value="38"></div>
<div class="field"><label for="hcSerialTxd">TX pin</label><input id="hcSerialTxd" type="number" min="0" max="48" value="39"></div>
<div class="field"><label for="hcSerialBaud">Baud</label><select id="hcSerialBaud"><option>115200</option><option>9600</option><option>19200</option><option>38400</option><option>57600</option><option>230400</option><option>460800</option><option>576000</option><option>921600</option></select></div>
<div class="field"><label for="hcSerialMode">Mode</label><select id="hcSerialMode"><option>PROTO</option><option>SIMPLE</option><option>TEXTMSG</option><option>NMEA</option><option>CALTOPO</option><option>WS85</option><option>VE_DIRECT</option><option>MS_CONFIG</option><option>LOG</option><option>LOGTEXT</option></select></div>
<label class="checkrow full"><input id="hcSerialOverride" type="checkbox">Override console serial port</label>
<button class="primaryAction" onclick="applyHeltecSerial()">Apply Serial Module</button>
</div></div>
<div class="card"><h2>Device Behavior</h2><div class="formgrid">
<div class="field"><label for="hcRole">Node role</label><select id="hcRole"><option>CLIENT</option><option>CLIENT_MUTE</option><option>ROUTER</option><option>ROUTER_CLIENT</option><option>REPEATER</option><option>TRACKER</option><option>SENSOR</option><option>TAK</option><option>CLIENT_HIDDEN</option><option>LOST_AND_FOUND</option><option>TAK_TRACKER</option><option>ROUTER_LATE</option><option>CLIENT_BASE</option></select></div>
<div class="field"><label for="hcRebroadcast">Rebroadcast</label><select id="hcRebroadcast"><option>ALL</option><option>ALL_SKIP_DECODING</option><option>LOCAL_ONLY</option><option>KNOWN_ONLY</option><option>NONE</option><option>CORE_PORTNUMS_ONLY</option></select></div>
<div class="field"><label for="hcNodeInfo">Node info seconds</label><input id="hcNodeInfo" type="number" min="0" value="900"></div>
<div class="field"><label for="hcBuzzer">Buzzer</label><select id="hcBuzzer"><option>ALL_ENABLED</option><option>DISABLED</option><option>NOTIFICATIONS_ONLY</option><option>SYSTEM_ONLY</option><option>DIRECT_MSG_ONLY</option></select></div>
<label class="checkrow full"><input id="hcLedOff" type="checkbox">Disable heartbeat LED</label>
<button class="primaryAction" onclick="applyHeltecDevice()">Apply Device Behavior</button>
</div></div>
<div class="card"><h2>Position And GPS</h2><div class="formgrid">
<label class="checkrow"><input id="hcGpsEnabled" type="checkbox" checked>GPS enabled</label>
<label class="checkrow"><input id="hcFixedPosition" type="checkbox">Fixed position</label>
<div class="field"><label for="hcGpsMode">GPS mode</label><select id="hcGpsMode"><option>ENABLED</option><option>DISABLED</option><option>NOT_PRESENT</option></select></div>
<div class="field"><label for="hcPositionBroadcast">Broadcast seconds</label><input id="hcPositionBroadcast" type="number" min="0" value="900"></div>
<div class="field"><label for="hcGpsUpdate">GPS update seconds</label><input id="hcGpsUpdate" type="number" min="0" value="30"></div>
<div class="field"><label for="hcGpsAttempt">GPS attempt seconds</label><input id="hcGpsAttempt" type="number" min="0" value="0"></div>
<label class="checkrow"><input id="hcSmartPosition" type="checkbox" checked>Smart broadcast</label>
<div class="field"><label for="hcSmartMeters">Smart meters</label><input id="hcSmartMeters" type="number" min="0" value="100"></div>
<div class="field"><label for="hcSmartSecs">Smart seconds</label><input id="hcSmartSecs" type="number" min="0" value="30"></div>
<button class="primaryAction" onclick="applyHeltecPosition()">Apply Position</button>
</div></div>
<div class="card"><h2>Power</h2><div class="formgrid">
<label class="checkrow full"><input id="hcPowerSaving" type="checkbox">Power saving</label>
<div class="field"><label for="hcShutdown">Shutdown on battery seconds</label><input id="hcShutdown" type="number" min="0" value="0"></div>
<div class="field"><label for="hcWaitBt">Wait Bluetooth seconds</label><input id="hcWaitBt" type="number" min="0" value="60"></div>
<div class="field"><label for="hcSds">Super deep sleep seconds</label><input id="hcSds" type="number" min="0" value="0"></div>
<div class="field"><label for="hcLs">Light sleep seconds</label><input id="hcLs" type="number" min="0" value="300"></div>
<div class="field"><label for="hcWake">Minimum wake seconds</label><input id="hcWake" type="number" min="0" value="10"></div>
<button class="primaryAction" onclick="applyHeltecPower()">Apply Power</button>
</div></div>
<div class="card"><h2>Channel Config</h2><div class="formgrid">
<div class="field"><label for="hcChanIndex">Channel index</label><input id="hcChanIndex" type="number" min="0" max="7" value="0"></div>
<div class="field"><label for="hcChanRole">Role</label><select id="hcChanRole"><option>PRIMARY</option><option>SECONDARY</option><option>DISABLED</option></select></div>
<div class="field"><label for="hcChanName">Name</label><input id="hcChanName" maxlength="11" placeholder="Channel name"></div>
<div class="field"><label for="hcChanPsk">PSK</label><input id="hcChanPsk" placeholder="Blank to keep"></div>
<label class="checkrow"><input id="hcChanUplink" type="checkbox">Uplink enabled</label>
<label class="checkrow"><input id="hcChanDownlink" type="checkbox">Downlink enabled</label>
<button onclick="applyHeltecChannel()">Apply Channel</button><button onclick="disableHeltecChannel()">Disable Channel</button>
<button onclick="heltecCmd('get channel')" class="full">Refresh Channel Config</button>
</div><div class="hint">Choose an index, set role/name/key options, then save after applying.</div></div>
<div class="card"><h2>Quick Commands</h2><div class="actions">
<button onclick="heltecCmd('get config')">Get Config</button><button onclick="heltecCmd('get module')">Get Modules</button><button onclick="heltecCmd('get owner')">Get Owner</button><button onclick="heltecCmd('get lora')">Get LoRa</button>
<button onclick="heltecAction('/heltec/save','Save command sent')">Save Config</button><button class="dangerAction" onclick="heltecAction('/heltec/reboot','Restart command sent')">Restart Heltec</button>
</div></div></section>
<section class="page" id="diagnostics"><div class="card"><h2>Health</h2><div class="stats" id="diagStats"></div><div class="actions"><button onclick="saveSnapshot()">Save Snapshot</button><button onclick="downloadSnapshot()">Download Live Snapshot</button><button onclick="fetch('/config',{method:'POST'})">Request Config</button></div></div><div class="card"><h2>Packet Counters</h2><div class="stats" id="packetStats"></div></div><div class="card"><h2>Last Serial Data</h2><pre id="diagSerial"></pre></div></section>
<section class="page" id="serialPage"><div class="card"><h2>Serial Link</h2><input id="cmd" placeholder="Serial command"><button onclick="sendCmd()">Send Command</button><pre id="serialLog" style="margin-top:6px"></pre></div></section>
<section class="page" id="logs"><div class="card"><h2>Event Log</h2><pre id="eventLog"></pre></div></section>
</main><script src="https://unpkg.com/leaflet@1.9.4/dist/leaflet.js"></script><script>
let leafletMap=null,markers={},realMapReady=false,currentPage='overview';
for(const b of document.querySelectorAll('#tabs button'))b.onclick=()=>showPage(b.dataset.page);
function showPage(id){currentPage=id;document.querySelectorAll('.page').forEach(p=>p.classList.toggle('active',p.id===id));document.querySelectorAll('#tabs button').forEach(b=>b.classList.toggle('active',b.dataset.page===id));if(id==='map'){setTimeout(()=>{initRealMap();if(leafletMap)leafletMap.invalidateSize()},80)}}
function initRealMap(){if(realMapReady||!window.L)return;leafletMap=L.map('realMap',{zoomControl:true,attributionControl:true});L.tileLayer('https://tile.openstreetmap.org/{z}/{x}/{y}.png',{maxZoom:19,attribution:'&copy; OpenStreetMap'}).addTo(leafletMap);leafletMap.setView([0,0],2);realMapReady=true;}
function setCfgValue(id,v){const e=document.getElementById(id);if(!e||document.activeElement===e||v===undefined||v===null)return;if(e.type==='checkbox')e.checked=!!v;else e.value=v;}
function setCfgSelect(id,v){const e=document.getElementById(id);if(!e||document.activeElement===e||v===undefined||v===null)return;const value=String(v);if(![...e.options].some(o=>o.value===value||o.text===value))e.add(new Option(value,value));e.value=value;}
function applyCurrentConfig(s){const c=s.heltecConfig;if(!c)return;if(c.lora&&c.lora.valid){setCfgSelect('hcRegion',c.lora.region);setCfgSelect('hcPreset',c.lora.preset);setCfgValue('hcHop',c.lora.hop);setCfgValue('hcTx',c.lora.txPower)}if(c.serial&&c.serial.valid){setCfgValue('hcSerialEnabled',c.serial.enabled);setCfgValue('hcSerialEcho',c.serial.echo);setCfgValue('hcSerialRxd',c.serial.rxd);setCfgValue('hcSerialTxd',c.serial.txd);setCfgSelect('hcSerialBaud',c.serial.baud);setCfgSelect('hcSerialMode',c.serial.mode);setCfgValue('hcSerialOverride',c.serial.override)}if(c.device&&c.device.valid){setCfgSelect('hcRole',c.device.role);setCfgSelect('hcRebroadcast',c.device.rebroadcast);setCfgValue('hcNodeInfo',c.device.nodeInfo);if(c.device.tz)setCfgValue('hcTz',c.device.tz);setCfgValue('hcLedOff',c.device.ledOff);setCfgSelect('hcBuzzer',c.device.buzzer)}if(c.position&&c.position.valid){setCfgValue('hcGpsEnabled',c.position.gpsEnabled);setCfgSelect('hcGpsMode',c.position.gpsMode);setCfgValue('hcFixedPosition',c.position.fixed);setCfgValue('hcSmartPosition',c.position.smart);setCfgValue('hcPositionBroadcast',c.position.broadcast);setCfgValue('hcGpsUpdate',c.position.gpsUpdate);setCfgValue('hcGpsAttempt',c.position.gpsAttempt);setCfgValue('hcSmartMeters',c.position.smartMeters);setCfgValue('hcSmartSecs',c.position.smartSecs)}if(c.power&&c.power.valid){setCfgValue('hcPowerSaving',c.power.saving);setCfgValue('hcShutdown',c.power.shutdown);setCfgValue('hcWaitBt',c.power.waitBt);setCfgValue('hcSds',c.power.sds);setCfgValue('hcLs',c.power.ls);setCfgValue('hcWake',c.power.wake)}}
async function refresh(){const s=await (await fetch('/status')).json();document.title=s.title||'LoRa Interface';appTitle.textContent=s.title||'LoRa Interface';ip.textContent=s.wifiMode+' '+s.ip;
stats.innerHTML=[['Node',s.myNode],['S3 Battery',s.battery+'% '+s.voltage+'V'],['Power',s.powerState],['Frames',s.frames],['Errors',s.errors],['RX/TX',s.rx+'/'+s.tx],['Nodes',s.online+'/'+s.total],['SD',s.sdStatus]]
.map(x=>`<div class=stat><div class=label>${x[0]}</div><div class=value>${x[1]}</div></div>`).join('');
storageStats.innerHTML=[['Status',s.sdAvailable?'mounted':s.sdStatus],['Type',s.sdType],['Used',formatBytes(s.sdUsedKb)],['Total',formatBytes(s.sdTotalKb)],['Writes',s.sdWrites],['Errors',s.sdErrors],['Map Cache',s.mapCacheLoaded?'loaded':s.mapCacheStatus]]
.map(x=>`<div class=stat><div class=label>${x[0]}</div><div class=value>${x[1]}</div></div>`).join('');
const hc=s.heltecConfig||{};
configRef.innerHTML=[['Node',s.myNode],['Name',s.myNodeName],['LoRa',hc.lora&&hc.lora.valid?`${hc.lora.region} ${hc.lora.preset} hop ${hc.lora.hop}`:'not loaded'],['Serial',hc.serial&&hc.serial.valid?`${hc.serial.enabled?'on':'off'} ${hc.serial.mode} ${hc.serial.baud}`:'not loaded'],['Device',hc.device&&hc.device.valid?`${hc.device.role} ${hc.device.rebroadcast}`:'not loaded'],['Position',hc.position&&hc.position.valid?`${hc.position.gpsMode} ${hc.position.broadcast}s`:'not loaded'],['Power',hc.power&&hc.power.valid?`${hc.power.saving?'saving':'normal'} wake ${hc.power.wake}s`:'not loaded'],['Config age',hc.ageSec>=0?hc.ageSec+'s':'none'],['Module age',hc.moduleAgeSec>=0?hc.moduleAgeSec+'s':'none']]
.map(x=>`<div class=stat><div class=label>${x[0]}</div><div class=value>${x[1]}</div></div>`).join('');
applyCurrentConfig(s);
diagStats.innerHTML=[['WiFi',s.wifiEnabled?`${s.wifiMode} ${s.ip}`:'off'],['Stations',s.wifiStations],['Battery',`${s.battery}% ${s.voltage}V`],['Power',s.powerState],['Battery trend',s.batteryTrend+' mV/min'],['SD',s.sdAvailable?'mounted':s.sdStatus],['GPS',s.gpsValid?`${s.gpsLat.toFixed(5)}, ${s.gpsLon.toFixed(5)}`:'no fix'],['Positioned nodes',s.positionedNodes],['Private channel',s.privateChannel>=0?s.privateChannel:'none'],['Map cache',s.mapCacheLoaded?'loaded':s.mapCacheStatus]]
.map(x=>`<div class=stat><div class=label>${x[0]}</div><div class=value>${x[1]}</div></div>`).join('');
packetStats.innerHTML=[['Frames',s.frames],['Decode errors',s.errors],['Stream frames',s.streamFrames],['Bad lengths',s.badLengths],['Text',s.textPackets],['Telemetry',s.telemetryPackets],['Positions',s.positionPackets],['Remote positions',s.remotePositionPackets],['Node info',s.nodeInfoPackets],['Config',s.configFrames],['Encrypted',s.encryptedPackets],['Other',s.otherFrames],['RX/TX packets',s.rx+'/'+s.tx],['RX/TX bytes',s.bytes+'/'+s.txBytes]]
.map(x=>`<div class=stat><div class=label>${x[0]}</div><div class=value>${x[1]}</div></div>`).join('');
channelRef.innerHTML=(s.channels&&s.channels.length?s.channels.map(c=>`<tr><td>${c.index}</td><td>${c.role||'-'}</td><td>${c.name||'-'}</td><td>${c.enabled?'enabled':'disabled'}</td><td>${c.uplink?'yes':'-'}</td><td>${c.downlink?'yes':'-'}</td><td>${c.pskSize?c.pskSize+' bytes':'-'}</td></tr>`).join(''):'<tr><td colspan="7">No channel config received yet</td></tr>');
publicChatLog.textContent=s.publicChat||'No public chat yet';familyChatLog.textContent=s.familyChat||'No family chat yet';directChatLog.textContent=s.directChat||'No direct messages yet';eventLog.textContent=s.log||'Waiting for radio data';
serialLog.textContent=`RX bytes: ${s.bytes}\nTX bytes: ${s.txBytes}\nLast byte: ${s.lastByte}\nMagic 94/C3: ${s.magic1}/${s.magic2}\nStream frames: ${s.streamFrames}\nBad lengths: ${s.badLengths}\nText/Tel/GPS/Node: ${s.textPackets}/${s.telemetryPackets}/${s.positionPackets}/${s.nodeInfoPackets}\nRemote GPS mapped: ${s.remotePositionPackets}\nConfig/Other/Encrypted: ${s.configFrames}/${s.otherFrames}/${s.encryptedPackets}\nLast port: ${s.lastPort}\nASCII seen: ${s.serialPeek||''}`;
diagSerial.textContent=`Last byte: ${s.lastByte}\nLast port: ${s.lastPort}\nASCII seen: ${s.serialPeek||''}\nLocal GPS bytes: ${s.localGpsBytes}\nGPS sentences with fix: ${s.localGpsSentences}\nGPS checksum failures: ${s.localGpsFailedChecksum}`;
nodesBody.innerHTML=s.nodes.map(n=>`<tr><td>${n.num}</td><td>${n.name}</td><td>${n.snr}</td><td>${n.age}s</td><td>${n.hasPosition?`${n.lat.toFixed(5)}, ${n.lon.toFixed(5)}`:'-'}</td></tr>`).join('');
if(currentPage==='map')drawMap(s);
}
async function send(){const m=msg.value.trim();if(!m)return;await fetch('/send',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'msg='+encodeURIComponent(m)+'&channel='+encodeURIComponent(chatChannel.value)});msg.value='';refresh();}
async function sendCmd(){const c=cmd.value.trim();if(!c)return;await fetch('/serial_cmd',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'cmd='+encodeURIComponent(c)});cmd.value='';refresh();}
async function mountSd(){await fetch('/sd/mount',{method:'POST'});refresh();}
async function saveSnapshot(){snapshotStatus.textContent='Saving snapshot...';const r=await fetch('/sd/snapshot',{method:'POST'});snapshotStatus.textContent=r.ok?'Snapshot saved to SD':'Snapshot save failed';refresh();}
async function downloadSnapshot(){const s=await (await fetch('/status')).json();const blob=new Blob([JSON.stringify(s,null,2)],{type:'application/json'});const a=document.createElement('a');a.href=URL.createObjectURL(blob);a.download='heltec_status_snapshot.json';a.click();URL.revokeObjectURL(a.href);}
async function heltecCmd(c,label){hcStatus.textContent='Sending...';const r=await fetch('/serial_cmd',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'cmd='+encodeURIComponent(c)});hcStatus.textContent=r.ok?(label||'Applied: '+c):'Failed: '+c;}
async function heltecAction(url,label){hcStatus.textContent='Sending...';const r=await fetch(url,{method:'POST'});hcStatus.textContent=r.ok?label:'Failed';setTimeout(refresh,400);}
async function heltecPost(url,fields,label){hcStatus.textContent='Sending...';const body=new URLSearchParams(fields);const r=await fetch(url,{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body});hcStatus.textContent=r.ok?label:'Failed';setTimeout(refresh,400);}
async function heltecBatch(cmds){for(const c of cmds){await heltecCmd(c);await new Promise(r=>setTimeout(r,180));}}
function applyHeltecLora(){heltecPost('/heltec/lora',{region:hcRegion.value,preset:hcPreset.value,hop:hcHop.value,tx:hcTx.value},'Radio settings sent');}
function applyHeltecSerial(){heltecPost('/heltec/serial',{enabled:hcSerialEnabled.checked?'1':'0',echo:hcSerialEcho.checked?'1':'0',rxd:hcSerialRxd.value,txd:hcSerialTxd.value,baud:hcSerialBaud.value,mode:hcSerialMode.value,override:hcSerialOverride.checked?'1':'0'},'Serial module sent');}
function applyHeltecName(){const n=hcName.value.trim();if(n)heltecPost('/heltec/name',{name:n,short:hcShortName.value.trim()},'Name update sent');}
function applyHeltecTime(){const z=hcTz.value.trim();if(z)heltecPost('/heltec/timezone',{tz:z},'Timezone update sent');}
function applyHeltecDevice(){heltecPost('/heltec/device',{role:hcRole.value,rebroadcast:hcRebroadcast.value,nodeInfo:hcNodeInfo.value,tz:hcTz.value.trim(),ledOff:hcLedOff.checked?'1':'0',buzzer:hcBuzzer.value},'Device behavior sent');}
function applyHeltecPosition(){heltecPost('/heltec/position',{gpsEnabled:hcGpsEnabled.checked?'1':'0',gpsMode:hcGpsMode.value,fixed:hcFixedPosition.checked?'1':'0',smart:hcSmartPosition.checked?'1':'0',broadcast:hcPositionBroadcast.value,gpsUpdate:hcGpsUpdate.value,gpsAttempt:hcGpsAttempt.value,smartMeters:hcSmartMeters.value,smartSecs:hcSmartSecs.value},'Position config sent');}
function applyHeltecPower(){heltecPost('/heltec/power',{saving:hcPowerSaving.checked?'1':'0',shutdown:hcShutdown.value,waitBt:hcWaitBt.value,sds:hcSds.value,ls:hcLs.value,wake:hcWake.value},'Power config sent');}
function channelIndex(){let i=parseInt(hcChanIndex.value||'0',10);if(isNaN(i)||i<0)i=0;if(i>7)i=7;hcChanIndex.value=i;return i;}
function applyHeltecChannel(){heltecPost('/heltec/channel',{index:channelIndex(),role:hcChanRole.value,name:hcChanName.value.trim(),psk:hcChanPsk.value.trim(),uplink:hcChanUplink.checked?'1':'0',downlink:hcChanDownlink.checked?'1':'0'},'Channel settings sent');}
function disableHeltecChannel(){heltecPost('/heltec/channel',{index:channelIndex(),role:'DISABLED'},'Disable channel sent');}
function formatBytes(kb){if(!kb)return'0 KB';return kb>=1024?Math.ceil(kb/1024)+' MB':kb+' KB';}
function drawMap(s){initRealMap();let pts=s.nodes.filter(n=>n.hasPosition);if(s.gpsValid){const live={num:s.myNode,name:(s.myNodeName||'Me')+' live',snr:0,age:0,hasPosition:true,lat:s.gpsLat,lon:s.gpsLon,alt:0,positionAge:0};const i=pts.findIndex(n=>n.num===s.myNode);if(i>=0)pts[i]=live;else pts=[live,...pts]}if(realMapReady){realMap.classList.remove('hidden');mapFallback.classList.add('hidden');drawRealMap(s,pts);return}realMap.classList.add('hidden');mapFallback.classList.remove('hidden');drawFallbackMap(s,pts);}
function drawRealMap(s,pts){if(!pts.length){mapMeta.textContent='No node positions yet';return}const seen={};for(const p of pts){seen[p.num]=true;const html=`<b>${p.name||p.num}</b><br>${p.num}<br>${p.lat.toFixed(6)}, ${p.lon.toFixed(6)}<br>Alt ${p.alt} m<br>${p.positionAge}s old`;if(!markers[p.num])markers[p.num]=L.circleMarker([p.lat,p.lon],{radius:7,color:p.num===s.myNode?'#00c985':'#68ffc0',weight:2,fillColor:p.num===s.myNode?'#00c985':'#68ffc0',fillOpacity:.85}).addTo(leafletMap);else markers[p.num].setLatLng([p.lat,p.lon]);markers[p.num].setStyle({color:p.num===s.myNode?'#00c985':'#68ffc0',fillColor:p.num===s.myNode?'#00c985':'#68ffc0'});markers[p.num].bindPopup(html)}for(const id in markers){if(!seen[id]){leafletMap.removeLayer(markers[id]);delete markers[id]}}const bounds=L.latLngBounds(pts.map(p=>[p.lat,p.lon]));leafletMap.fitBounds(bounds.pad(.2),{maxZoom:15,animate:false});mapMeta.textContent=`OpenStreetMap | ${pts.length} positioned node${pts.length===1?'':'s'}`;}
function drawFallbackMap(s,pts){const c=mapFallback,ctx=c.getContext('2d');ctx.clearRect(0,0,c.width,c.height);ctx.fillStyle='#07100d';ctx.fillRect(0,0,c.width,c.height);ctx.strokeStyle='#1f3d35';ctx.lineWidth=1;for(let x=40;x<c.width;x+=80){ctx.beginPath();ctx.moveTo(x,0);ctx.lineTo(x,c.height);ctx.stroke()}for(let y=40;y<c.height;y+=80){ctx.beginPath();ctx.moveTo(0,y);ctx.lineTo(c.width,y);ctx.stroke()}if(!pts.length){ctx.fillStyle='#8ab7a6';ctx.font='16px system-ui';ctx.fillText('Waiting for position packets',24,40);mapMeta.textContent='No node positions yet';return}let minLat=Math.min(...pts.map(p=>p.lat)),maxLat=Math.max(...pts.map(p=>p.lat)),minLon=Math.min(...pts.map(p=>p.lon)),maxLon=Math.max(...pts.map(p=>p.lon));let latSpan=Math.max(maxLat-minLat,0.000001),lonSpan=Math.max(maxLon-minLon,0.000001),pad=28;for(const p of pts){let x=pad+(p.lon-minLon)*(c.width-pad*2)/lonSpan,y=pad+(maxLat-p.lat)*(c.height-pad*2)/latSpan;ctx.fillStyle=p.num===s.myNode?'#00c985':'#68ffc0';ctx.beginPath();ctx.arc(x,y,6,0,Math.PI*2);ctx.fill();ctx.fillStyle='#e8fff5';ctx.font='12px system-ui';ctx.fillText(p.name||p.num,x+9,y+4)}mapMeta.textContent=`Offline plot | ${pts.length} positioned node${pts.length===1?'':'s'} | center ${((minLat+maxLat)/2).toFixed(5)}, ${((minLon+maxLon)/2).toFixed(5)}`;}
setInterval(refresh,1000);refresh();
</script></body></html>
)HTML";
