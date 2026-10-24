#line 1 "C:\\Users\\justi\\Documents\\Arduino\\s3-lora-interface\\src\\WebUi.h"
#pragma once

#include <Arduino.h>

static const char WEB_UI_HTML[] PROGMEM = R"HTML(
<!doctype html><html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>LoRa Interface</title>
<link rel="stylesheet" href="/assets/leaflet-1.9.4.css">
<style>
:root{color-scheme:dark;--bg:#07111f;--panel:#102238;--input:#09182a;--border:#294766;--text:#f4f8ff;--muted:#a1b5ce;--accent:#85bfff;--action:#3b9dff;--ink:#04192e}
*{box-sizing:border-box}html,body{min-height:100%;background:var(--bg);color:var(--text)}body{margin:0;font:14px/1.5 system-ui,-apple-system,"Segoe UI",sans-serif}
[hidden],.hidden{display:none!important}header{position:sticky;top:0;z-index:5;padding:12px max(16px,calc((100vw - 1100px)/2));background:#081523;border-bottom:1px solid var(--border)}
.top{display:flex;align-items:center;justify-content:space-between;gap:8px;padding-bottom:4px}h1{font-size:20px;letter-spacing:-.03em;margin:0}.connectionMeta{text-align:right;font-size:11px;color:var(--muted)}#ip{font-size:12px}
nav{display:grid;grid-template-columns:repeat(6,minmax(0,1fr));gap:6px;padding-top:10px}nav button,nav summary{width:100%;min-height:44px;font:600 13px system-ui;text-align:center;border-radius:8px;margin:0;padding:12px 8px;cursor:pointer;background:var(--panel);color:var(--text);border:1px solid var(--border)}
nav button.active{background:var(--action);color:var(--ink);border-color:var(--action)}.navTools{position:relative}.navTools summary{list-style:none}.navTools summary::-webkit-details-marker{display:none}.navTools summary:after{content:' ▾';color:var(--muted)}.navTools[open] summary,.navTools summary.active{border-color:var(--accent);color:var(--accent)}
.toolsMenu{position:absolute;right:0;top:calc(100% + 8px);width:220px;background:var(--panel);padding:8px;border:1px solid var(--border);border-radius:10px;box-shadow:0 12px 32px #0008;z-index:10;display:grid;gap:5px}.toolsMenu button{text-align:left}.menuCaption{padding:4px 10px;color:var(--muted);font-size:11px;text-transform:uppercase;letter-spacing:.1em}
main{padding:22px 16px 40px;max-width:1100px;margin:0 auto}section.page{display:none}.page.active{display:block}.card{background:var(--panel);border:1px solid var(--border);border-radius:12px;padding:18px;margin-bottom:16px}h2{font-size:17px;line-height:1.3;margin:0 0 14px}h3{font-size:15px;margin:0 0 12px}
.pageIntro{display:flex;justify-content:space-between;align-items:center;gap:16px;margin:0 0 20px}.pageIntro h2{font-size:27px;letter-spacing:-.04em;margin:0 0 6px}.pageIntro p{margin:0;color:var(--muted);font-size:13px}.pageIntro .eyebrow{font-size:10px;letter-spacing:.18em;font-weight:700;color:var(--accent);margin:0 0 8px}.pageIntro .actions{flex-shrink:0;flex-wrap:nowrap}.pageIntro .actions button{flex:0 0 auto}
.stats{display:grid;grid-template-columns:repeat(auto-fit,minmax(145px,1fr));gap:10px}.stat{padding:14px;border-radius:9px;background:var(--input);border:1px solid #203a57}.label{color:var(--accent);font-size:12px;margin-bottom:5px}.value{font-size:19px;font-weight:600;line-height:1.3;overflow-wrap:anywhere}
input,select,textarea{box-sizing:border-box;width:100%;padding:11px;background:var(--input);border:1px solid #39638d;color:var(--text);border-radius:8px}button,input,select,textarea{font:inherit;min-height:44px}button{margin-top:6px;padding:10px 12px;border:1px solid transparent;border-radius:8px;background:var(--action);color:var(--ink);font-weight:650;cursor:pointer;transition:filter .1s ease}button:hover{filter:brightness(1.12)}button:active,button.clicked{filter:brightness(1.2)}button:disabled{cursor:wait;opacity:.55}
button:focus-visible,summary:focus-visible,a:focus-visible,input:focus-visible,select:focus-visible,textarea:focus-visible{outline:2px solid var(--accent);outline-offset:3px}.secondaryAction,.dangerAction{background:#193753;color:var(--text);border:1px solid #39638d}.dangerAction{border-color:#a35765;color:#ffbdc7}
.actions{display:flex;gap:8px;flex-wrap:wrap}.actions button{flex:1 1 130px}.actions a{display:inline-flex;align-items:center;justify-content:center;min-height:44px;padding:10px 12px;border:1px solid var(--border);border-radius:8px;margin-top:6px}.formgrid{display:grid;grid-template-columns:1fr 1fr;gap:12px}.field{display:flex;flex-direction:column;gap:5px;min-width:0}.field label{color:var(--accent);font-size:13px}.full,.primaryAction{grid-column:1/-1}
.hint,.minihelp{color:var(--muted);font-size:12px;line-height:1.5}.hint{margin-top:8px}.statusbox{min-height:34px;background:var(--input);border:1px solid var(--border);border-radius:8px;padding:8px;color:var(--muted)}.checkrow{display:flex;align-items:center;gap:8px;color:var(--text);font-size:13px}.checkrow input{width:20px;min-height:20px;accent-color:var(--action)}
a{color:var(--accent);text-decoration:none}a:hover{text-decoration:underline}.links{display:grid;grid-template-columns:1fr 1fr;gap:8px}.links a{padding:10px 8px;min-height:44px;border:1px solid #39638d;border-radius:8px;background:var(--input);text-align:center}.choicebar{display:grid;grid-template-columns:1fr 1fr;gap:8px;margin-bottom:8px}
table{width:100%;border-collapse:collapse;font-size:13px}td,th{border-bottom:1px solid var(--border);padding:11px 8px;text-align:left;vertical-align:top}th{color:var(--accent);font-size:11px;text-transform:uppercase;letter-spacing:.05em}td{overflow-wrap:anywhere}tbody tr{cursor:pointer}tbody tr:hover,.favoriteRow{background:#163652}.tablewrap{overflow:auto}.tablewrap>table:not(.deliveryTable){min-width:640px}.tablewrap>table:not(.deliveryTable) td{white-space:nowrap}
.miniAction{margin:0;padding:6px 8px;border-radius:5px;font-size:11px;white-space:nowrap}.miniAction.secondary{background:#193753;color:var(--text);border:1px solid #39638d}.deliveryTable td:nth-child(2){min-width:160px;max-width:380px}.deliveryTable tbody tr{cursor:default}.deliveryBadge{display:inline-block;padding:3px 9px;border-radius:999px;font-size:11px;font-weight:700;white-space:nowrap;border:1px solid #496583;color:#d3e3f9;background:#172b42}.deliveryBadge.ack{background:#173d68;color:#b3daff;border-color:#397ab6}.deliveryBadge.failed{background:#3c1c20;color:#ffb7be;border-color:#8a3c48}.deliveryBadge.pending{background:#302811;color:#f8d88d;border-color:#74602b}.deliveryMeta{display:block;font-size:11px;color:var(--muted);margin-top:5px}
pre{white-space:pre-wrap;overflow:auto;max-height:62vh;margin:0;color:#e8f2ff;background:var(--input);border-radius:8px;padding:14px;font:13px/1.55 ui-monospace,Consolas,monospace}#radioHealth{margin-top:6px;padding:7px 10px;font-size:12px;min-height:0;background:#0a1c30}.warning,#radioHealth.warning{background:#2b2311;border-color:#a47c2e;color:#ffdc8c}
details.disclosure>summary,.overviewDetails>summary{font-weight:600;font-size:16px;cursor:pointer;min-height:36px;color:var(--text)}details.disclosure[open]>summary,.overviewDetails[open]>summary{margin-bottom:16px}.disclosure .stats{margin-top:12px}.disclosure .minihelp{margin:8px 0 12px}
.settingsSearch{display:block;margin-bottom:18px}.settingsSearch>span{display:block;color:var(--accent);font-size:12px;margin-bottom:6px}.settingsSearch input{max-width:560px}.settingsCrumbs{display:flex;align-items:center;gap:10px;flex-wrap:wrap;margin-bottom:16px}.settingsCrumbs button{margin:0;background:transparent;color:var(--accent);border:1px solid var(--border);font-size:13px}.settingsCrumbs span{color:var(--muted)}
.categoryGrid,.sectionGrid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:12px}.settingsChoice{width:100%;margin:0;display:flex;flex-direction:column;align-items:flex-start;gap:7px;padding:20px;text-align:left;color:var(--text);background:var(--panel);border:1px solid var(--border);border-radius:12px;min-height:138px}.settingsChoice:hover{border-color:var(--accent);background:#15304c}.settingsChoice strong{font-size:17px}.settingsChoice small{font-size:12px;font-weight:400;line-height:1.5;color:var(--muted)}.settingsChoice .choiceMeta{font-size:11px;color:var(--accent);letter-spacing:.04em}.sectionGrid .settingsChoice{min-height:115px}.settingsPanel{max-width:850px}.settingsPanel>h2{color:var(--text);font-size:21px}.settingsDescription{margin:-5px 0 18px;color:var(--muted);font-size:13px}.configStatus{margin-bottom:18px;font-size:12px;color:var(--muted)}.flowline{display:none}.moduleFeedback{max-width:850px;margin-bottom:16px}.searchSummary{margin:0 0 12px;color:var(--muted)}
body.setup-required nav button:not([data-page="config"]),body.setup-required #navTools,body.setup-required section.page:not(#config),body.setup-required .settingsSearch,body.setup-required #settingsDirectory,body.setup-required #settingsBreadcrumb{display:none}body.setup-required #config{display:block}
#realMap{width:100%;height:58vh;min-height:320px;background:var(--input);border:1px solid var(--border);border-radius:8px;overflow:hidden}#mapFallback{width:100%;height:320px;background:var(--input);border:1px solid var(--border);border-radius:8px}.mapMeta{color:var(--muted);font-size:12px;margin-top:8px}#realMap canvas{border:0;border-radius:0}#map .row{display:flex;gap:8px;flex-wrap:wrap;margin-bottom:10px}#map .row button{flex:1 1 100px;margin-top:0;background:#193753;color:var(--text);border:1px solid #39638d}
#realMap .leaflet-control-attribution{max-width:68%;font-size:9px;background:#102238dd;color:#d0e2f6}#realMap .leaflet-control-attribution a{color:var(--accent)}.leaflet-container{background:var(--input);color:#10233d}.leaflet-popup-content-wrapper,.leaflet-popup-tip{background:var(--panel);color:var(--text)}
@media(max-width:560px){header{padding:10px 12px}h1{font-size:17px;overflow-wrap:anywhere}nav{grid-template-columns:repeat(3,minmax(0,1fr))}main{padding:16px 12px 28px}.pageIntro{display:block}.pageIntro h2{font-size:25px}.pageIntro .actions{margin-top:12px}.pageIntro .actions button{flex:1}.card{padding:14px;border-radius:10px}.stats{grid-template-columns:repeat(2,minmax(0,1fr))}.stat{padding:11px}.value{font-size:17px}.formgrid,.categoryGrid,.sectionGrid{grid-template-columns:1fr}.full{grid-column:1/-1}.settingsChoice{min-height:110px;padding:16px}.links{grid-template-columns:1fr}.toolsMenu{width:min(240px,calc(100vw - 24px))}#realMap{height:52vh}.deliveryTable,.deliveryTable tbody{display:block;width:100%}.deliveryTable thead{display:none}.deliveryTable tr{display:grid;grid-template-columns:minmax(0,1fr) auto;gap:8px;padding:12px 0;border-bottom:1px solid var(--border)}.deliveryTable td{display:block;border:0;padding:0;min-width:0!important;max-width:none!important}.deliveryTable td:nth-child(1){grid-column:1;font-weight:600}.deliveryTable td:nth-child(2){grid-column:1/-1;grid-row:2}.deliveryTable td:nth-child(3){grid-column:1}.deliveryTable td:nth-child(4){grid-column:2;grid-row:3}.deliveryTable button{margin:0}}
@media(prefers-reduced-motion:reduce){button{transition:none}}
.advancedFields{margin:4px 0 8px;border:1px solid var(--border);border-radius:8px;padding:12px}.advancedFields>summary{cursor:pointer;color:var(--accent);font-size:13px;font-weight:600;min-height:24px}.advancedFields[open]>summary{margin-bottom:14px}.settingsCrumbs button:only-child{margin:0}
</style></head><body><header><div class="top"><h1 id="appTitle">LoRa Interface</h1><div class="connectionMeta"><div id="ip"></div><div id="statusFreshness">Connecting…</div></div></div><div class="hint statusbox" id="radioHealth">Heltec: waiting for status</div>
<nav id="tabs" aria-label="Main navigation"><button data-page="overview" class="active" aria-current="page">Overview</button><button data-page="chat">Messages</button><button data-page="nodes">Nodes</button><button data-page="map">Map</button><button data-page="config">Settings</button><details id="navTools" class="navTools"><summary id="toolsLabel">Tools</summary><div class="toolsMenu"><span class="menuCaption">Maintenance</span><button data-page="diagnostics">Health & diagnostics</button><button data-page="storage">SD storage</button><button data-page="logs">Event logs</button><button data-page="serialPage">Serial console</button><button data-page="firmware">Firmware update</button></div></details></nav></header><main>
<section class="page active" id="overview"><div class="pageIntro"><div><p class="eyebrow">FIELD CONSOLE</p><h2>Your mesh at a glance</h2><p>Radio connection, battery and nearby nodes.</p></div><div class="actions"><button onclick="showPage('chat')">Open messages</button><button class="secondaryAction" onclick="showPage('map')">View map</button></div></div><div class="card"><h2>Live status</h2><div class="stats" id="stats"></div><button onclick="requestConfig()">Refresh Config</button></div><div class="card"><details class="overviewDetails"><summary>Channels &amp; radio configuration</summary><div class="stats" id="configRef"></div><div class="hint statusbox" id="channelPairStatus">Secondary channel status updates from device config.</div><div class="tablewrap"><table><thead><tr><th>Index</th><th>Role</th><th>Name</th><th>State</th><th>GPS</th><th>Up</th><th>Down</th><th>Key</th></tr></thead><tbody id="channelRef"></tbody></table></div></details></div></section>
<section class="page" id="chat"><div class="card"><h2>Send Message</h2><div class="formgrid"><div class="field"><label for="chatChannel">Channel</label><select id="chatChannel"><option value="ch:0">Primary</option><option value="ch:1">Channel 1</option></select></div><div class="field"><label for="msg">Message</label><input id="msg" maxlength="233" placeholder="Message to mesh"></div><button class="primaryAction" onclick="send()">Send</button></div><div class="hint statusbox" id="chatStatus">Channel names update when config is loaded.</div></div><div class="card"><h2>Direct Message</h2><div class="formgrid"><div class="field"><label for="directTo">To node</label><input id="directTo" maxlength="9" placeholder="!1234ABCD"></div><div class="field"><label for="directMsg">Message</label><input id="directMsg" maxlength="233" placeholder="Direct message"></div><button class="primaryAction" onclick="sendDirect()">Send Direct</button></div><div class="hint statusbox" id="directStatus">Use a node ID from the Nodes table.</div></div><div class="card deliveryCard"><h2>Message delivery</h2><div class="minihelp">Sent to radio means accepted by its queue. Direct ACK confirms the destination radio; mesh ACK confirms a broadcast was relayed. Neither is a read receipt. Unconfirmed messages may have arrived.</div><div class="tablewrap"><table class="deliveryTable"><thead><tr><th>Destination</th><th>Message</th><th>Status</th><th>Retry</th></tr></thead><tbody id="txHistoryBody"></tbody></table></div><div class="hint statusbox" id="txRetryStatus">Retries become available after failure or 2 minutes without confirmation. A retry can create a duplicate.</div></div><div class="card"><h2 id="chatPublicTitle">Primary Chat</h2><pre id="publicChatLog"></pre></div><div class="card"><h2 id="chatFamilyTitle">Secondary Channel Chat</h2><pre id="familyChatLog"></pre></div><div class="card"><h2>Direct</h2><pre id="directChatLog"></pre></div></section>
<section class="page" id="map"><div class="card"><h2>Offline map</h2><div class="row"><button type="button" onclick="centerOfflineMap()">My location</button><button type="button" onclick="fitOfflineNodes()">Show nodes</button><button type="button" onclick="reloadOfflineMap()">Refresh maps</button></div><div id="realMap"></div><canvas id="mapFallback" class="hidden" width="640" height="360"></canvas><div class="mapMeta" id="mapMeta" role="status" aria-live="polite">Maps load from the device SD card.</div><div class="hint">Drag to explore. Detailed tiles fall back to coarser maps where available. Map data: installed SD tiles (USGS / OpenStreetMap contributors).</div></div><div class="card"><h2>Touchscreen coverage</h2><div class="hint" id="mapCoverageStatus">Open Map on the touchscreen to check this area.</div><div class="stats" id="mapCoverageStats"></div></div></section>
<section class="page" id="nodes"><div class="card"><h2>Nodes</h2><div class="actions"><button onclick="clearNodes()">Clear NodeDB</button><button onclick="requestConfig()">Refresh Config</button></div><div class="formgrid"><div class="field"><label for="nodeSearch">Search nodes</label><input id="nodeSearch" placeholder="Name or !nodeid"></div><label class="checkrow"><input id="nodeFavoritesOnly" type="checkbox">Favorites only</label></div><div class="minihelp" id="nodeFilterStatus">Favorites sort to the top.</div><div class="hint statusbox" id="nodesStatus">Nodes update from fresh radio traffic and Heltec config refreshes.</div><div class="tablewrap"><table><thead><tr><th>Node</th><th>Name</th><th>SNR</th><th>Age</th><th>GPS</th><th>Favorite</th></tr></thead><tbody id="nodesBody"></tbody></table></div></div></section>
<section class="page" id="storage"><div class="pageIntro"><div><p class="eyebrow">DEVICE STORAGE</p><h2>SD storage</h2><p>Storage usage, saved files and snapshots.</p></div></div>
<div class="card"><h2>Storage status</h2><div class="stats" id="storageStats"></div><div class="actions"><button onclick="saveSnapshot()">Save snapshot</button><a href="/diagnostics/report">Download field report</a></div><div class="hint statusbox" id="snapshotStatus">Snapshots include status, counters, node list, channel summary, logs, and GPS/SD health.</div></div>
<details class="card disclosure"><summary>Saved logs & files</summary><div class="links"><a href="/sd/events">Events</a><a href="/sd/public">Public chat</a><a href="/sd/secondary">Secondary chat</a><a href="/sd/positions">Positions CSV</a><a href="/sd/status-snapshot">Status snapshot</a><a href="/sd/last-location">Last GPS</a><a href="/sd/mapcache">Map cache</a></div></details>
<details class="card disclosure"><summary>Storage diagnostics & maintenance</summary><div class="stats" id="storageDetailStats"></div><div class="actions"><button class="secondaryAction" onclick="mountSd()">Remount SD</button><button class="secondaryAction" onclick="downloadSnapshot()">Download live snapshot</button></div></details></section>
<section class="page" id="config">
<div class="pageIntro"><div><p class="eyebrow">DEVICE SETTINGS</p><h2>Settings</h2><p>Choose a category or search for a setting.</p></div><div class="actions"><button class="secondaryAction" onclick="requestConfig()">Refresh from Heltec</button></div></div>
<div class="configStatus" id="configWorkflow" role="status">Waiting for current settings from the Heltec.</div><div id="configFlowLine" class="flowline"></div>
<label class="settingsSearch" for="settingsSearch"><span>Find a setting</span><input id="settingsSearch" type="search" placeholder="Try Wi-Fi, GPS or sleep" autocomplete="off"></label>
<div id="settingsBreadcrumb" class="settingsCrumbs" aria-label="Settings path" hidden><button type="button" id="settingsBackRoot">All settings</button><span id="settingsDivider" aria-hidden="true" hidden>/</span><button type="button" id="settingsBackCategory" hidden></button></div>
<div id="settingsDirectory"><p id="settingsSearchSummary" class="searchSummary" role="status" hidden></p><div id="settingsCategories" class="categoryGrid"></div><div id="settingsResults" class="sectionGrid" hidden></div><p id="settingsEmpty" class="card" hidden>No matching settings. Try a different name, such as GPS, Wi-Fi or power.</p></div>
<div class="hint statusbox moduleFeedback" id="moduleStatus" role="status" hidden>Module settings apply to the Heltec.</div>
<div class="card settingsPanel" id="cfgRadio" hidden><h2>Radio And Identity</h2><div class="formgrid">
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
<div class="card settingsPanel" id="cfgInterface" hidden><h2>Interface Settings</h2><div class="formgrid">
<div class="field"><label for="ifaceName">Device name</label><input id="ifaceName" maxlength="32" placeholder="jjs-node"></div>
<div class="field"><label for="ifaceHost">Hostname</label><input id="ifaceHost" maxlength="32" placeholder="Same as device name"></div>
<div class="field"><label for="ifaceApSsid">AP network name</label><input id="ifaceApSsid" maxlength="32" placeholder="Same as device name"></div>
<div class="field"><label for="ifaceApChannel">AP channel</label><input id="ifaceApChannel" type="number" min="1" max="13" value="6"></div>
<div class="field"><label for="ifaceWebUser">Web login user</label><input id="ifaceWebUser" maxlength="32"></div>
<div class="field full"><label for="ifaceWebPass">New web and AP password</label><input id="ifaceWebPass" type="password" minlength="8" placeholder="Leave blank to keep"><div class="minihelp">This password is used for both the WiFi AP and Web UI login.</div></div>
<button class="primaryAction" onclick="applyInterfaceSettings()">Save Interface Settings</button>
</div><div class="hint statusbox" id="ifaceStatus">These settings stay on this device after OTA, so future firmware bins can be shared.</div></div>
<div class="card settingsPanel" id="cfgSerial" hidden><h2>Serial Module</h2><div class="formgrid">
<label class="checkrow"><input id="hcSerialEnabled" type="checkbox" checked>Enabled</label>
<div class="field"><label for="hcSerialBaud">Baud</label><select id="hcSerialBaud"><option>115200</option><option>9600</option><option>19200</option><option>38400</option><option>57600</option><option>230400</option><option>460800</option><option>576000</option><option>921600</option></select></div>
<div class="field"><label for="hcSerialMode">Mode</label><select id="hcSerialMode"><option>PROTO</option><option>SIMPLE</option><option>TEXTMSG</option><option>NMEA</option><option>CALTOPO</option><option>WS85</option><option>VE_DIRECT</option><option>MS_CONFIG</option><option>LOG</option><option>LOGTEXT</option></select></div>
<details class="full advancedFields"><summary>Pins & advanced serial settings</summary><div class="formgrid">
<label class="checkrow"><input id="hcSerialEcho" type="checkbox">Echo</label>
<div class="field"><label for="hcSerialRxd">RX pin</label><input id="hcSerialRxd" type="number" min="0" max="48" value="38"></div>
<div class="field"><label for="hcSerialTxd">TX pin</label><input id="hcSerialTxd" type="number" min="0" max="48" value="39"></div>
<div class="field"><label for="hcSerialTimeout">Timeout ms</label><input id="hcSerialTimeout" type="number" min="0" value="0"></div>
<label class="checkrow full"><input id="hcSerialOverride" type="checkbox">Override console serial port</label>
</div></details>
<button class="primaryAction" onclick="applyHeltecSerial()">Apply Serial Module</button>
</div></div>
<div class="card settingsPanel" id="cfgMqtt" hidden><h2>MQTT Module</h2><div class="formgrid">
<label class="checkrow full"><input id="hcMqttEnabled" type="checkbox">MQTT enabled</label>
<div class="field"><label for="hcMqttAddress">Broker address</label><input id="hcMqttAddress" maxlength="63" placeholder="mqtt.meshtastic.org"></div>
<div class="field"><label for="hcMqttRoot">Root topic</label><input id="hcMqttRoot" maxlength="31" placeholder="msh"></div>
<div class="field"><label for="hcMqttUser">Username</label><input id="hcMqttUser" maxlength="63"></div>
<div class="field"><label for="hcMqttPass">Password</label><input id="hcMqttPass" type="password" maxlength="31" placeholder="Leave blank to keep"></div>
<label class="checkrow"><input id="hcMqttEncrypt" type="checkbox">Encrypted MQTT</label>
<label class="checkrow"><input id="hcMqttTls" type="checkbox">TLS</label>
<details class="full advancedFields"><summary>Advanced MQTT settings</summary><div class="formgrid">
<label class="checkrow"><input id="hcMqttJson" type="checkbox">JSON packets</label>
<label class="checkrow"><input id="hcMqttProxy" type="checkbox">Proxy to client</label>
<label class="checkrow full"><input id="hcMqttMap" type="checkbox">Map reporting</label>
</div></details>
<button class="primaryAction" onclick="applyMqtt()">Apply MQTT</button>
</div><div class="hint statusbox" id="mqttStatus">Refresh from Heltec to load MQTT status.</div></div>
<div class="card settingsPanel" id="cfgNotify" hidden><h2>External Notification</h2><div class="formgrid">
<label class="checkrow full"><input id="hcNotifyEnabled" type="checkbox">Enable external notification</label>
<label class="checkrow"><input id="hcNotifyMsg" type="checkbox">Alert on message</label>
<label class="checkrow"><input id="hcNotifyBell" type="checkbox">Alert on bell</label>
<details class="full advancedFields"><summary>Outputs & advanced alerts</summary><div class="formgrid">
<div class="field"><label for="hcNotifyOutput">Output GPIO</label><input id="hcNotifyOutput" type="number" min="0" max="48" value="0"></div>
<div class="field"><label for="hcNotifyVibra">Vibra GPIO</label><input id="hcNotifyVibra" type="number" min="0" max="48" value="0"></div>
<div class="field"><label for="hcNotifyBuzzer">Buzzer GPIO</label><input id="hcNotifyBuzzer" type="number" min="0" max="48" value="0"></div>
<div class="field"><label for="hcNotifyMs">Output ms</label><input id="hcNotifyMs" type="number" min="0" value="1000"></div>
<div class="field"><label for="hcNotifyNag">Nag seconds</label><input id="hcNotifyNag" type="number" min="0" value="0"></div>
<label class="checkrow"><input id="hcNotifyActive" type="checkbox">Active high</label>
<label class="checkrow"><input id="hcNotifyMsgVibra" type="checkbox">Message vibra</label>
<label class="checkrow"><input id="hcNotifyMsgBuzzer" type="checkbox">Message buzzer</label>
<label class="checkrow"><input id="hcNotifyBellVibra" type="checkbox">Bell vibra</label>
<label class="checkrow"><input id="hcNotifyBellBuzzer" type="checkbox">Bell buzzer</label>
<label class="checkrow"><input id="hcNotifyPwm" type="checkbox">Use PWM buzzer</label>
<label class="checkrow"><input id="hcNotifyI2s" type="checkbox">Use I2S as buzzer</label>
</div></details>
<button class="primaryAction" onclick="applyNotify()">Apply Notification</button>
</div></div>
<div class="card settingsPanel" id="cfgTelemetry" hidden><h2>Telemetry Module</h2><div class="formgrid">
<label class="checkrow"><input id="hcTeleDevice" type="checkbox">Device telemetry</label>
<label class="checkrow"><input id="hcTeleEnv" type="checkbox">Environment</label>
<label class="checkrow"><input id="hcTeleAir" type="checkbox">Air quality</label>
<label class="checkrow"><input id="hcTelePower" type="checkbox">Power telemetry</label>
<label class="checkrow"><input id="hcTeleHealth" type="checkbox">Health telemetry</label>
<details class="full advancedFields"><summary>Reporting intervals & screens</summary><div class="formgrid">
<div class="field"><label for="hcTeleDeviceSecs">Device seconds</label><input id="hcTeleDeviceSecs" type="number" min="0" value="0"></div>
<label class="checkrow"><input id="hcTeleEnvScreen" type="checkbox">Env screen</label>
<label class="checkrow"><input id="hcTeleEnvF" type="checkbox">Fahrenheit</label>
<div class="field"><label for="hcTeleEnvSecs">Env seconds</label><input id="hcTeleEnvSecs" type="number" min="0" value="0"></div>
<label class="checkrow"><input id="hcTeleAirScreen" type="checkbox">Air screen</label>
<div class="field"><label for="hcTeleAirSecs">Air seconds</label><input id="hcTeleAirSecs" type="number" min="0" value="0"></div>
<label class="checkrow"><input id="hcTelePowerScreen" type="checkbox">Power screen</label>
<div class="field"><label for="hcTelePowerSecs">Power seconds</label><input id="hcTelePowerSecs" type="number" min="0" value="0"></div>
<label class="checkrow"><input id="hcTeleHealthScreen" type="checkbox">Health screen</label>
<div class="field"><label for="hcTeleHealthSecs">Health seconds</label><input id="hcTeleHealthSecs" type="number" min="0" value="0"></div>
</div></details>
<button class="primaryAction" onclick="applyTelemetry()">Apply Telemetry</button>
</div></div>
<div class="card settingsPanel" id="cfgStore" hidden><h2>Store & forward</h2><div class="formgrid">
<label class="checkrow"><input id="hcStoreEnabled" type="checkbox">Store and forward</label>
<label class="checkrow"><input id="hcStoreHeartbeat" type="checkbox">S&F heartbeat</label>
<label class="checkrow"><input id="hcStoreServer" type="checkbox">S&F server</label>
<div class="field"><label for="hcStoreRecords">S&F records</label><input id="hcStoreRecords" type="number" min="0" value="0"></div>
<div class="field"><label for="hcStoreMax">History max</label><input id="hcStoreMax" type="number" min="0" value="0"></div>
<div class="field"><label for="hcStoreWindow">History window</label><input id="hcStoreWindow" type="number" min="0" value="0"></div>
<button onclick="applyStoreForward()">Apply Store & Forward</button></div></div>
<div class="card settingsPanel" id="cfgRange" hidden><h2>Range test</h2><div class="formgrid">
<label class="checkrow"><input id="hcRangeEnabled" type="checkbox">Range test</label>
<label class="checkrow"><input id="hcRangeSave" type="checkbox">Range save file</label>
<label class="checkrow"><input id="hcRangeClear" type="checkbox">Clear range file on boot</label>
<div class="field"><label for="hcRangeSender">Sender seconds</label><input id="hcRangeSender" type="number" min="0" value="0"></div>
<button onclick="applyRangeTest()">Apply Range Test</button></div></div>
<div class="card settingsPanel" id="cfgNeighbor" hidden><h2>Neighbor information</h2><div class="formgrid">
<label class="checkrow"><input id="hcNeighborEnabled" type="checkbox">Neighbor info</label>
<label class="checkrow"><input id="hcNeighborLora" type="checkbox">Transmit over LoRa</label>
<div class="field"><label for="hcNeighborSecs">Neighbor seconds</label><input id="hcNeighborSecs" type="number" min="0" value="0"></div>
<button onclick="applyNeighbor()">Apply Neighbor Info</button></div></div>
<div class="card settingsPanel" id="cfgCanned" hidden><h2>Canned messages</h2><div class="formgrid">
<label class="checkrow"><input id="hcCannedEnabled" type="checkbox">Canned messages</label>
<label class="checkrow"><input id="hcCannedRotary" type="checkbox">Rotary input</label>
<label class="checkrow"><input id="hcCannedUpdown" type="checkbox">Up/down input</label>
<label class="checkrow"><input id="hcCannedBell" type="checkbox">Send bell</label>
<div class="field"><label for="hcCannedSource">Input source</label><input id="hcCannedSource" maxlength="15" placeholder="_any"></div>
<div class="field"><label for="hcCannedPinA">Pin A</label><input id="hcCannedPinA" type="number" min="0" max="48" value="0"></div>
<div class="field"><label for="hcCannedPinB">Pin B</label><input id="hcCannedPinB" type="number" min="0" max="48" value="0"></div>
<div class="field"><label for="hcCannedPinPress">Pin press</label><input id="hcCannedPinPress" type="number" min="0" max="48" value="0"></div>
<button onclick="applyCanned()">Apply Canned Message</button></div></div>
<div class="card settingsPanel" id="cfgAudio" hidden><h2>Heltec voice audio</h2><div class="formgrid">
<label class="checkrow"><input id="hcAudioEnabled" type="checkbox">Codec2 audio</label>
<div class="field"><label for="hcAudioRate">Audio bitrate</label><select id="hcAudioRate"><option value="0">Default</option><option value="1">3200</option><option value="2">2400</option><option value="3">1600</option><option value="4">1400</option><option value="5">1300</option><option value="6">1200</option><option value="7">700</option><option value="8">700B</option></select></div>
<div class="field"><label for="hcAudioPtt">PTT GPIO</label><input id="hcAudioPtt" type="number" min="0" max="48" value="0"></div>
<div class="field"><label for="hcAudioWs">I2S WS</label><input id="hcAudioWs" type="number" min="0" max="48" value="0"></div>
<div class="field"><label for="hcAudioSd">I2S SD</label><input id="hcAudioSd" type="number" min="0" max="48" value="0"></div>
<div class="field"><label for="hcAudioDin">I2S DIN</label><input id="hcAudioDin" type="number" min="0" max="48" value="0"></div>
<div class="field"><label for="hcAudioSck">I2S SCK</label><input id="hcAudioSck" type="number" min="0" max="48" value="0"></div>
<button onclick="applyAudio()">Apply Audio</button></div></div>
<div class="card settingsPanel" id="cfgAmbient" hidden><h2>Ambient LED</h2><div class="formgrid">
<label class="checkrow"><input id="hcAmbientEnabled" type="checkbox">Ambient LED</label>
<div class="field"><label for="hcAmbientCurrent">LED current</label><input id="hcAmbientCurrent" type="number" min="0" max="31" value="10"></div>
<div class="field"><label for="hcAmbientRed">Red</label><input id="hcAmbientRed" type="number" min="0" max="255" value="0"></div>
<div class="field"><label for="hcAmbientGreen">Green</label><input id="hcAmbientGreen" type="number" min="0" max="255" value="0"></div>
<div class="field"><label for="hcAmbientBlue">Blue</label><input id="hcAmbientBlue" type="number" min="0" max="255" value="0"></div>
<button onclick="applyAmbient()">Apply Ambient LED</button></div></div>
<div class="card settingsPanel" id="cfgDetection" hidden><h2>Detection sensor</h2><div class="formgrid">
<label class="checkrow"><input id="hcDetectEnabled" type="checkbox">Detection sensor</label>
<label class="checkrow"><input id="hcDetectBell" type="checkbox">Detection bell</label>
<label class="checkrow"><input id="hcDetectPullup" type="checkbox">Use pullup</label>
<div class="field"><label for="hcDetectName">Sensor name</label><input id="hcDetectName" maxlength="19"></div>
<div class="field"><label for="hcDetectPin">Monitor pin</label><input id="hcDetectPin" type="number" min="0" max="48" value="0"></div>
<div class="field"><label for="hcDetectTrigger">Trigger</label><select id="hcDetectTrigger"><option value="0">Logic low</option><option value="1">Logic high</option><option value="2">Falling edge</option><option value="3">Rising edge</option><option value="4">Either edge active low</option><option value="5">Either edge active high</option></select></div>
<div class="field"><label for="hcDetectMin">Minimum seconds</label><input id="hcDetectMin" type="number" min="0" value="0"></div>
<div class="field"><label for="hcDetectState">State seconds</label><input id="hcDetectState" type="number" min="0" value="0"></div>
<button onclick="applyDetection()">Apply Detection Sensor</button></div></div>
<div class="card settingsPanel" id="cfgPax" hidden><h2>Paxcounter</h2><div class="formgrid">
<label class="checkrow"><input id="hcPaxEnabled" type="checkbox">Paxcounter</label>
<div class="field"><label for="hcPaxSecs">Pax seconds</label><input id="hcPaxSecs" type="number" min="0" value="0"></div>
<div class="field"><label for="hcPaxWifi">WiFi threshold</label><input id="hcPaxWifi" type="number" value="-80"></div>
<div class="field"><label for="hcPaxBle">BLE threshold</label><input id="hcPaxBle" type="number" value="-80"></div>
<button onclick="applyPax()">Apply Paxcounter</button></div></div>
<div class="card settingsPanel" id="cfgStatus" hidden><h2>Status message</h2><div class="formgrid">
<div class="field full"><label for="hcStatusMessage">Status message</label><input id="hcStatusMessage" maxlength="79"></div>
<button onclick="applyStatusMessage()">Apply Status Message</button></div></div>

<div class="card settingsPanel" id="cfgDevice" hidden><h2>Device Behavior</h2><div class="formgrid">
<div class="field"><label for="hcRole">Node role</label><select id="hcRole"><option>CLIENT</option><option>CLIENT_MUTE</option><option>ROUTER</option><option>ROUTER_CLIENT</option><option>REPEATER</option><option>TRACKER</option><option>SENSOR</option><option>TAK</option><option>CLIENT_HIDDEN</option><option>LOST_AND_FOUND</option><option>TAK_TRACKER</option><option>ROUTER_LATE</option><option>CLIENT_BASE</option></select></div>
<div class="field"><label for="hcRebroadcast">Rebroadcast</label><select id="hcRebroadcast"><option>ALL</option><option>ALL_SKIP_DECODING</option><option>LOCAL_ONLY</option><option>KNOWN_ONLY</option><option>NONE</option><option>CORE_PORTNUMS_ONLY</option></select></div>
<div class="field"><label for="hcNodeInfo">Node info seconds</label><input id="hcNodeInfo" type="number" min="0" value="900"></div>
<div class="field"><label for="hcBuzzer">Buzzer</label><select id="hcBuzzer"><option>ALL_ENABLED</option><option>DISABLED</option><option>NOTIFICATIONS_ONLY</option><option>SYSTEM_ONLY</option><option>DIRECT_MSG_ONLY</option></select></div>
<label class="checkrow full"><input id="hcLedOff" type="checkbox">Disable heartbeat LED</label>
<button class="primaryAction" onclick="applyHeltecDevice()">Apply Device Behavior</button>
</div></div>
<div class="card settingsPanel" id="cfgPosition" hidden><h2>Position And GPS</h2><div class="formgrid">
<label class="checkrow"><input id="hcFixedPosition" type="checkbox">Fixed position</label>
<div class="field"><label for="hcGpsMode">GPS mode</label><select id="hcGpsMode"><option>ENABLED</option><option>DISABLED</option><option>NOT_PRESENT</option></select></div>
<div class="field"><label for="hcPositionBroadcast">Broadcast seconds</label><input id="hcPositionBroadcast" type="number" min="0" value="900"></div>
<label class="checkrow"><input id="hcSmartPosition" type="checkbox" checked>Smart broadcast</label>
<details class="full advancedFields"><summary>GPS timing & advanced settings</summary><div class="formgrid">
<label class="checkrow"><input id="hcGpsEnabled" type="checkbox" checked>GPS enabled</label>
<div class="field"><label for="hcGpsUpdate">GPS update seconds</label><input id="hcGpsUpdate" type="number" min="0" value="30"></div>
<div class="field"><label for="hcGpsAttempt">GPS attempt seconds</label><input id="hcGpsAttempt" type="number" min="0" value="0"></div>
<div class="field"><label for="hcSmartMeters">Smart meters</label><input id="hcSmartMeters" type="number" min="0" value="100"></div>
<div class="field"><label for="hcSmartSecs">Smart seconds</label><input id="hcSmartSecs" type="number" min="0" value="30"></div>
</div></details>
<button class="primaryAction" onclick="applyHeltecPosition()">Apply Position</button>
</div></div>
<div class="card settingsPanel" id="cfgPower" hidden><h2>Power</h2><div class="formgrid">
<label class="checkrow full"><input id="hcPowerSaving" type="checkbox">Power saving</label>
<div class="field"><label for="hcShutdown">Shutdown on battery seconds</label><input id="hcShutdown" type="number" min="0" value="0"></div>
<details class="full advancedFields"><summary>Sleep & wake timings</summary><div class="formgrid">
<div class="field"><label for="hcWaitBt">Wait Bluetooth seconds</label><input id="hcWaitBt" type="number" min="0" value="60"></div>
<div class="field"><label for="hcSds">Super deep sleep seconds</label><input id="hcSds" type="number" min="0" value="0"></div>
<div class="field"><label for="hcLs">Light sleep seconds</label><input id="hcLs" type="number" min="0" value="300"></div>
<div class="field"><label for="hcWake">Minimum wake seconds</label><input id="hcWake" type="number" min="0" value="10"></div>
</div></details>
<button class="primaryAction" onclick="applyHeltecPower()">Apply Power</button>
</div></div>
<div class="card settingsPanel" id="cfgDisplay" hidden><h2>Display</h2><div class="formgrid">
<div class="field"><label for="hcDisplayScreen">Screen on seconds</label><input id="hcDisplayScreen" type="number" min="0" value="60"></div>
<div class="field"><label for="hcDisplayUnits">Units</label><select id="hcDisplayUnits"><option value="0">Metric</option><option value="1">Imperial</option></select></div>
<label class="checkrow"><input id="hcDisplayFlip" type="checkbox">Flip screen</label>
<label class="checkrow"><input id="hcDisplayWake" type="checkbox">Wake on motion</label>
<label class="checkrow"><input id="hcDisplay12h" type="checkbox">12 hour clock</label>
<label class="checkrow"><input id="hcDisplayLongName" type="checkbox">Long node names</label>
<label class="checkrow"><input id="hcDisplayBubbles" type="checkbox">Message bubbles</label>
<details class="full advancedFields"><summary>Advanced display settings</summary><div class="formgrid">
<div class="field"><label for="hcDisplayCarousel">Carousel seconds</label><input id="hcDisplayCarousel" type="number" min="0" value="0"></div>
<div class="field"><label for="hcDisplayOled">OLED type</label><select id="hcDisplayOled"><option value="0">Auto</option><option value="1">SSD1306</option><option value="2">SH1106</option><option value="3">SH1107</option><option value="4">SH1107 128x128</option></select></div>
<div class="field"><label for="hcDisplayMode">Display mode</label><select id="hcDisplayMode"><option value="0">Default</option><option value="1">Two color</option><option value="2">Inverted</option><option value="3">Color</option></select></div>
<label class="checkrow"><input id="hcDisplayNorth" type="checkbox">Compass north top</label>
<label class="checkrow"><input id="hcDisplayBold" type="checkbox">Bold heading</label>
</div></details>
<button class="primaryAction" onclick="applyDisplay()">Apply Display</button>
</div></div>
<div class="card settingsPanel" id="cfgNetwork" hidden><h2>Heltec Network</h2><div class="formgrid">
<label class="checkrow full"><input id="hcNetWifi" type="checkbox">WiFi enabled on Heltec radio</label>
<div class="field"><label for="hcNetSsid">WiFi SSID</label><input id="hcNetSsid" maxlength="32"></div>
<div class="field"><label for="hcNetPsk">WiFi password</label><input id="hcNetPsk" type="password" maxlength="64" placeholder="Leave blank to keep"></div>
<div class="field"><label for="hcNetNtp">NTP server</label><input id="hcNetNtp" maxlength="32" placeholder="meshtastic.pool.ntp.org"></div>
<div class="field"><label for="hcNetSyslog">Syslog server</label><input id="hcNetSyslog" maxlength="32"></div>
<label class="checkrow"><input id="hcNetEth" type="checkbox">Ethernet enabled</label>
<label class="checkrow"><input id="hcNetIpv6" type="checkbox">IPv6 enabled</label>
<label class="checkrow"><input id="hcNetUdp" type="checkbox">UDP broadcast</label>
<button class="primaryAction" onclick="applyNetwork()">Apply Heltec Network</button>
</div><div class="hint statusbox" id="networkStatus">This configures the attached Heltec radio network, not the CYD Web UI network.</div></div>
<div class="card settingsPanel" id="cfgBluetooth" hidden><h2>Bluetooth</h2><div class="formgrid">
<label class="checkrow full"><input id="hcBtEnabled" type="checkbox">Bluetooth enabled on Heltec radio</label>
<div class="field"><label for="hcBtMode">Pairing mode</label><select id="hcBtMode"><option value="0">Random PIN</option><option value="1">Fixed PIN</option><option value="2">No PIN</option></select></div>
<div class="field"><label for="hcBtPin">Fixed PIN</label><input id="hcBtPin" type="number" min="0" max="999999" value="123456"></div>
<button class="primaryAction" onclick="applyBluetooth()">Apply Bluetooth</button>
</div></div>
<div class="card settingsPanel" id="cfgChannel" hidden><h2>Channel Config</h2><div class="choicebar">
<div class="field"><label for="hcChanPreset">Quick setup</label><select id="hcChanPreset" onchange="applyChannelPreset()"><option value="">Choose preset...</option><option value="primary">Primary channel</option><option value="secondary">Secondary channel</option><option value="disable">Disable selected channel</option></select></div>
<div class="field"><label for="hcChanIndex">Edit channel</label><select id="hcChanIndex" onchange="loadChannelEditor(this.value)"><option value="0">0 - Primary</option><option value="1">1 - Secondary</option><option value="2">2</option><option value="3">3</option><option value="4">4</option><option value="5">5</option><option value="6">6</option><option value="7">7</option></select></div>
</div><div class="formgrid">
<div class="field"><label for="hcChanRole">Channel type</label><select id="hcChanRole"><option value="PRIMARY">Primary channel</option><option value="SECONDARY">Secondary channel</option><option value="DISABLED">Disabled</option></select><div class="minihelp">Only one primary channel should exist. Extra chat groups are secondary channels.</div></div>
<div class="field"><label for="hcChanName">Channel name</label><input id="hcChanName" maxlength="11" placeholder="Name from device"></div>
<div class="field full"><label for="hcChanPsk">Key / PSK</label><input id="hcChanPsk" placeholder="PSK status unknown until channel config is loaded"><div class="minihelp" id="hcChanPskStatus">PSK status unknown until channel config is loaded.</div><div class="minihelp">Two devices can communicate on this channel when the channel name and PSK match. Entering a value here replaces the Heltec's current key.</div></div>
<div class="field full"><label for="hcChanLocation">GPS location on this channel</label><select id="hcChanLocation"><option value="32">Share exact GPS</option><option value="16">Share approximate GPS</option><option value="0">Hide GPS location</option></select><div class="minihelp">This controls the channel's position precision. Hide GPS keeps your position out of packets on this channel.</div></div>
<label class="checkrow"><input id="hcChanUplink" type="checkbox">Uplink to MQTT</label>
<label class="checkrow"><input id="hcChanDownlink" type="checkbox">Downlink from MQTT</label>
<button class="primaryAction" onclick="applyHeltecChannel()">Apply Channel Changes</button>
<button onclick="disableHeltecChannel()">Disable Selected Channel</button><button onclick="heltecAction('/heltec/save','Save command sent')">Save To Heltec</button>
<button onclick="channelEditorDirty=false;heltecCmd('get channel','Refresh command sent')" class="full">Refresh From Heltec</button>
</div><div class="hint statusbox" id="channelHelp">Click a row in Current Config or choose a preset. Apply sends the change; Save To Heltec stores it after testing.</div></div>
</section>
<section class="page" id="diagnostics"><div class="pageIntro"><div><p class="eyebrow">DEVICE HEALTH</p><h2>Status & diagnostics</h2><p>The essentials first. Open a topic for detailed readings.</p></div><div class="actions"><a href="/diagnostics/report">Download field report</a></div></div>
<div class="card"><h2>Current status</h2><div class="stats" id="diagStats"></div></div>
<details class="card disclosure"><summary>Battery & runtime</summary><div class="stats" id="diagBatteryStats"></div></details>
<details class="card disclosure"><summary>GPS & position</summary><div class="stats" id="diagGpsStats"></div></details>
<details class="card disclosure"><summary>Radio traffic & serial data</summary><div class="stats" id="packetStats"></div><h3>Last serial data</h3><pre id="diagSerial"></pre></details>
<details class="card disclosure"><summary>Maps & tile cache</summary><div class="stats" id="diagMapStats"></div></details>
<details class="card disclosure"><summary>Memory, Wi-Fi & storage</summary><div class="stats" id="diagSystemStats"></div></details>
<details class="card disclosure"><summary>Audio & hardware checks</summary><div class="stats" id="diagAudioStats"></div><div class="actions"><button onclick="testTone()">Test message sound</button><button class="secondaryAction" onclick="probeI2cMap()">Probe I2C map</button></div><div class="hint statusbox" id="toneStatus">Sound status appears after a test.</div><pre id="i2cProbeStatus">Run Probe I2C map to inspect the touch, codec and fuel gauge buses.</pre></details>
</section>
<section class="page" id="serialPage"><div class="card"><h2>Serial Link</h2><input id="cmd" placeholder="Serial command"><button onclick="sendCmd()">Send Command</button><pre id="serialLog" style="margin-top:6px"></pre></div><details class="card disclosure"><summary>Heltec commands</summary><div class="minihelp">Read configuration, save settings or restart the connected radio.</div><div class="statusbox" id="commandStatus" role="status">Command status appears here.</div><div class="actions">
<button onclick="heltecCmd('get config')">Get Config</button><button onclick="heltecCmd('get module')">Get Modules</button><button onclick="heltecCmd('get owner')">Get Owner</button><button onclick="heltecCmd('get lora')">Get LoRa</button>
<button onclick="heltecAction('/heltec/save','Save command sent')">Save Config</button><button class="dangerAction" onclick="heltecAction('/heltec/reboot','Restart command sent')">Restart Heltec</button>
</div></details></section>
<section class="page" id="firmware"><div class="card"><h2>Firmware</h2><div class="stats" id="firmwareStats"></div><form id="firmwareForm" class="formgrid" method="POST" action="/firmware" enctype="multipart/form-data"><div class="field full"><label for="firmwareFile">Firmware .bin</label><input id="firmwareFile" name="firmware" type="file" accept=".bin,application/octet-stream"></div><button class="primaryAction" type="submit">Upload Firmware</button></form><div class="hint statusbox" id="firmwareStatus">Choose a compiled .bin from the same partition scheme. The device will reboot after a successful upload.</div></div></section>
<section class="page" id="logs"><div class="card"><h2>Event Log</h2><pre id="eventLog"></pre></div></section>
</main><script src="/assets/leaflet-1.9.4.js"></script><script>
let leafletMap=null,markers={},realMapReady=false,currentPage='overview',currentChannels=[],currentNodes=[],channelEditorDirty=false;
document.addEventListener('click',e=>{const b=e.target.closest('button');if(!b)return;b.classList.remove('clicked');void b.offsetWidth;b.classList.add('clicked');setTimeout(()=>b.classList.remove('clicked'),180);});
for(const b of document.querySelectorAll('#tabs button'))b.onclick=()=>showPage(b.dataset.page);
function showPage(id){if(!document.getElementById(id))return;currentPage=id;document.querySelectorAll('.page').forEach(p=>p.classList.toggle('active',p.id===id));document.querySelectorAll('#tabs button').forEach(b=>{const active=b.dataset.page===id;b.classList.toggle('active',active);b.setAttribute('aria-current',active?'page':'false')});navTools.open=false;toolsLabel.classList.toggle('active',['diagnostics','storage','serialPage','firmware','logs'].includes(id));if(id==='map'){setTimeout(()=>{initRealMap();if(leafletMap)leafletMap.invalidateSize()},80)}}
function jumpToConfig(id){openSettingsPanel(id);}
const settingsGroups=[
 {id:'radio',title:'Radio & location',description:'Radio preset, channels and GPS position.'},
 {id:'device',title:'Device & screen',description:'CYD interface, Heltec behavior, display and power.'},
 {id:'connections',title:'Connections',description:'Heltec Wi-Fi, Bluetooth, serial link and MQTT.'},
 {id:'messages',title:'Messages & alerts',description:'Notifications, canned messages and message storage.'},
 {id:'sensors',title:'Sensors & tools',description:'Telemetry, attached hardware and mesh testing.'}
];
const settingsCatalog=[
 ['cfgRadio','radio','Radio & identity','Region, radio preset, transmit power and node names.'],
 ['cfgChannel','radio','Channels','Channel names, keys, privacy and uplink settings.'],
 ['cfgPosition','radio','GPS & position','Heltec position source, GPS mode and broadcast intervals.'],
 ['cfgInterface','device','CYD interface','Device name, Wi-Fi access point and web login.'],
 ['cfgDevice','device','Heltec behavior','Node role, rebroadcast policy and time zone.'],
 ['cfgDisplay','device','Heltec display','Screen timeout, units, orientation and appearance.'],
 ['cfgPower','device','Heltec power','Power saving, sleep, wake and shutdown intervals.'],
 ['cfgNetwork','connections','Heltec Wi-Fi','WiFi network, password, NTP, syslog and network services.'],
 ['cfgBluetooth','connections','Bluetooth','Enable Bluetooth, pairing mode and PIN.'],
 ['cfgSerial','connections','Serial link','PROTO link, baud rate, GPIO pins and timeout.'],
 ['cfgMqtt','connections','MQTT','Broker address, login, encryption and map reporting.'],
 ['cfgNotify','messages','External alerts','Heltec notification outputs, vibration and buzzer.'],
 ['cfgStore','messages','Store & forward','Message history, heartbeat and server mode.'],
 ['cfgCanned','messages','Canned messages','Preset message controls, input source and buttons.'],
 ['cfgStatus','messages','Status message','Text advertised as the node status message.'],
 ['cfgTelemetry','sensors','Telemetry','Device, environment, air, power and health readings.'],
 ['cfgDetection','sensors','Detection sensor','Monitor a GPIO pin and configure sensor alerts.'],
 ['cfgAmbient','sensors','Ambient LED','LED brightness and RGB color on attached hardware.'],
 ['cfgAudio','sensors','Heltec voice audio','Codec2 bitrate and voice hardware pins; separate from CYD notification sounds.'],
 ['cfgPax','sensors','Paxcounter','Wi-Fi and BLE device counting thresholds and interval.'],
 ['cfgNeighbor','sensors','Neighbor information','Neighbor reporting interval and LoRa transmission.'],
 ['cfgRange','sensors','Range test','Test transmission interval and range log options.']
].map(([id,group,title,description])=>({id,group,title,description,search:''}));
let settingsGroup='',settingsPanel='';
const settingsActionMessages={};
function settingsWords(value){return String(value||'').toLowerCase().replace(/[^a-z0-9]+/g,' ').trim().split(/\s+/).filter(Boolean);}
function settingsChoice(item,kind){return `<button type="button" class="settingsChoice" data-settings-${kind}="${h(item.id)}"><span class="choiceMeta">${kind==='group'?settingsCatalog.filter(x=>x.group===item.id).length+' sections':h(settingsGroups.find(g=>g.id===item.group).title)}</span><strong>${h(item.title)}</strong><small>${h(item.description)}</small></button>`;}
function renderSettingsDirectory(){
 const query=settingsWords(settingsSearch.value),searching=query.length>0;
 settingsDirectory.hidden=!!settingsPanel;
 settingsBreadcrumb.hidden=!settingsPanel&&!settingsGroup&&!searching;
 settingsBackCategory.hidden=!settingsGroup;
 settingsDivider.hidden=!settingsGroup;
 settingsBackCategory.textContent=settingsGroups.find(g=>g.id===settingsGroup)?.title||'';
 settingsCategories.hidden=!!settingsGroup||searching;
 settingsResults.hidden=!settingsGroup&&!searching;
 settingsSearchSummary.hidden=!searching;
 if(!settingsGroup&&!searching)settingsCategories.innerHTML=settingsGroups.map(x=>settingsChoice(x,'group')).join('');
 const matches=settingsCatalog.filter(x=>searching?query.every(word=>x.search.includes(word)):x.group===settingsGroup);
 settingsResults.innerHTML=matches.map(x=>settingsChoice(x,'panel')).join('');
 settingsEmpty.hidden=!searching||matches.length>0;
 settingsSearchSummary.textContent=searching?`${matches.length} matching ${matches.length===1?'section':'sections'}`:'';
 moduleStatus.hidden=!settingsPanel||settingsPanel==='cfgInterface';
}
function closeSettingsPanels(){for(const item of settingsCatalog){const panel=document.getElementById(item.id);if(panel)panel.hidden=true;}settingsPanel='';}
function openSettingsHome(){closeSettingsPanels();settingsGroup='';settingsSearch.value='';renderSettingsDirectory();}
function openSettingsGroup(id){if(!settingsGroups.some(g=>g.id===id))return;closeSettingsPanels();settingsGroup=id;settingsSearch.value='';renderSettingsDirectory();}
function openSettingsPanel(id){
 const item=settingsCatalog.find(x=>x.id===id);if(!item)return;
 const words=settingsWords(settingsSearch.value);
 closeSettingsPanels();settingsPanel=id;settingsGroup=item.group;settingsSearch.value='';
 const panel=document.getElementById(id);panel.hidden=false;
 moduleStatus.textContent=settingsActionMessages[id]||'Changes in this section apply to the Heltec.';
 for(const details of panel.querySelectorAll('details.advancedFields'))if(words.length&&words.every(word=>settingsWords(details.textContent).join(' ').includes(word)))details.open=true;
 renderSettingsDirectory();showPage('config');
 if(window.scrollTo)window.scrollTo({top:0,behavior:'auto'});
}
function initSettingsNavigation(){
 if(!document.querySelectorAll('.settingsPanel').length)return;
 for(const item of settingsCatalog){const panel=document.getElementById(item.id);if(!panel)continue;item.search=settingsWords(item.title+' '+item.description+' '+[...panel.querySelectorAll('label,.minihelp')].map(x=>x.textContent).join(' ')).join(' ');const description=document.createElement('p');description.className='settingsDescription';description.textContent=item.description;panel.querySelector('h2').after(description);}
 settingsSearch.addEventListener('input',()=>{closeSettingsPanels();settingsGroup='';renderSettingsDirectory();});
 settingsDirectory.addEventListener('click',event=>{const group=event.target.closest('[data-settings-group]'),panel=event.target.closest('[data-settings-panel]');if(group)openSettingsGroup(group.dataset.settingsGroup);if(panel)openSettingsPanel(panel.dataset.settingsPanel);});
 settingsBackRoot.onclick=openSettingsHome;
 settingsBackCategory.onclick=()=>openSettingsGroup(settingsGroup);
 openSettingsHome();
 document.addEventListener('keydown',event=>{if(event.key==='Escape')navTools.open=false;});
 document.addEventListener('click',event=>{if(!event.target.closest('#navTools'))navTools.open=false;});
}
initSettingsNavigation();

function renderStatGroup(id,items){const element=document.getElementById(id);if(element)element.innerHTML=items.map(([label,value])=>`<div class="stat"><div class="label">${h(label)}</div><div class="value">${h(value)}</div></div>`).join('');}
function renderHealth(s){
 renderStatGroup('diagStats',[
  ['Heltec',s.linkHealth||'waiting'],['Last valid response',s.lastRadioResponseSec==null?'never':`${s.lastRadioResponseSec}s ago`],
  ['Battery',s.fuelGaugePresent?`${s.battery}% · ${s.voltage} V`:'Gauge unavailable'],['Remaining',batteryRuntimeLabel(s)],
  ['GPS',s.localGpsFreshFix?'Live fix':s.gpsValid?'Last known position':'No fix'],['SD card',s.sdAvailable?'Mounted':s.sdStatus],
  ['Wi-Fi',s.wifiEnabled?`${s.wifiMode} · ${s.ip}`:'Off'],['Power',s.powerState]
 ]);
 renderStatGroup('diagBatteryStats',[
  ['Battery alert',s.batteryHealth||'ok'],['Discharge',`${s.batteryDischargePercentHr||0}%/hr`],
  ['Raw voltage',`${s.rawSenseVoltage||0} V`],['Voltage filter',s.batteryVoltageUnstable?'Settling':'Stable'],
  ['Voltage trend',`${s.batteryTrend||0} mV/min`],['Estimate basis',s.batteryRuntimeBasis||'Not yet available']
 ]);
 renderStatGroup('diagGpsStats',[
  ['Position',s.gpsValid&&Number.isFinite(s.gpsLat)&&Number.isFinite(s.gpsLon)?`${s.gpsLat.toFixed(5)}, ${s.gpsLon.toFixed(5)}`:'No fix'],
  ['Satellites used',s.localGpsSatsUsed??'Unknown'],['Satellites in view',s.localGpsSatsInView??'Unknown'],
  ['GPS sharing',s.gpsShareState||'Off'],['Sharing queued / accepted',`${s.gpsShareQueued||0} / ${s.gpsShareAccepted||0}`],
  ['Positioned nodes',s.positionedNodes||0],['Secondary channel',secondaryChatIndex(s)>=0?secondaryChatIndex(s):'None']
 ]);
 renderStatGroup('diagAudioStats',[
  ['Audio',`${s.toneAudioReady?'I2S ready':'No I2S'} / ${s.toneCodecReady?'Codec ready':'No codec'}`],
  ['Tone gain',`${s.toneVolume||0}% ${s.toneBoost||''} ${s.toneCodecProfile||''} reg ${s.toneCodecReg32??s.toneCodecReg}`],
  ['Tone writes',`${s.toneWriteCalls||0} calls · ${s.toneShortWrites||0} short · ${s.toneWriteFailures||0} failed`],
  ['Amp pin',`${s.toneAmpPin} · enable ${s.toneAmpEnableLevel}`]
 ]);
 renderStatGroup('diagSystemStats',[
  ['Internal heap',`${s.internalFreeKb||0} / ${s.internalLargestKb||0} KB`],['PSRAM',`${s.psramFreeKb||0} / ${s.psramLargestKb||0} KB`],
  ['Wi-Fi stations',s.wifiStations||0],['SD append wait / queued / dropped',`${s.sdAppendQueueWaiting||0} / ${s.sdAppendQueued||0} / ${s.sdAppendDropped||0}`]
 ]);
 renderStatGroup('diagMapStats',[
  ['Map cache',s.mapCacheLoaded?'Loaded':s.mapCacheStatus],['Tile root',s.mapTileRootFound?s.mapTileRoot:'Not found'],
  ['Tile cache',s.mapTileCacheEnabled?`${s.mapTileCacheResident||0} / ${s.mapTileCacheSlots} tiles · ${s.mapTileCacheKb} KiB PSRAM`:'Disabled'],
  ['Tile hits / misses',`${s.mapTileCacheHits||0} / ${s.mapTileCacheMisses||0}`],
  ['Tile SD reads',`${s.mapTileSdReadCalls||0} calls · ${Math.round((s.mapTileSdReadBytes||0)/1024)} KiB`],
  ['Tile read errors',s.mapTileReadErrors||0],['Map render last / max',`${s.mapRenderLastMs||0} / ${s.mapRenderMaxMs||0} ms`],
  ['Map worker start / apply / fail',`${s.mapRenderWorkerStarts||0} / ${s.mapRenderWorkerApplies||0} / ${s.mapRenderWorkerFailures||0}`]
 ]);
}

function h(v){return String(v===undefined||v===null?'':v).replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));}
let offlineLayer=null,offlineGeneration=null,offlineCentered=false,offlineStatus=null,offlinePoints=[];
const offlineQueue=[],offlineCache=new Map();let offlineBusy=false;
function offlineMapCenter(s,pts){const own=pts.find(p=>p.num===s.myNode);const p=own||(s.offlineMapCenter&&{lat:s.offlineMapCenter.lat,lon:s.offlineMapCenter.lon})||pts[0];return p&&Number.isFinite(p.lat)&&Number.isFinite(p.lon)?[p.lat,p.lon]:null;}
function centerOfflineMap(){if(!realMapReady||!offlineStatus)return;const center=offlineMapCenter(offlineStatus,offlinePoints);if(center){leafletMap.setView(center,Math.max(12,leafletMap.getZoom()),{animate:false});offlineCentered=true;}else mapMeta.textContent='Waiting for GPS or a saved location';}
function fitOfflineNodes(){if(!realMapReady||!offlinePoints.length)return;leafletMap.fitBounds(L.latLngBounds(offlinePoints.map(p=>[p.lat,p.lon])).pad(.2),{maxZoom:14,animate:false});offlineCentered=true;}
function reloadOfflineMap(){offlineCache.clear();if(offlineLayer)offlineLayer.redraw();}
function updateOfflineMapMeta(){
 if(!offlineLayer||!offlineCentered)return;
 const bounds=realMap.getBoundingClientRect();let ready=0,coarse=0,missing=0,failed=0,loading=0;
 for(const tile of offlineLayer.getContainer().querySelectorAll('canvas')){
  const b=tile.getBoundingClientRect();if(b.right<=bounds.left||b.left>=bounds.right||b.bottom<=bounds.top||b.top>=bounds.bottom)continue;
  if(tile.dataset.state==='ready')ready++;else if(tile.dataset.state==='coarse')coarse++;else if(tile.dataset.state==='missing')missing++;else if(tile.dataset.state==='error')failed++;else loading++;
 }
 mapMeta.textContent=`Offline SD maps · Zoom ${leafletMap.getZoom()} · ${ready} detailed${coarse?` · ${coarse} coarser`:''}${missing?` · ${missing} missing`:''}${failed?` · ${failed} unavailable`:''}${loading?` · ${loading} loading`:''}`;
}
function offlineTileNotice(tile,text,state){const ctx=tile.getContext('2d');ctx.fillStyle='#14251f';ctx.fillRect(0,0,256,256);ctx.strokeStyle='#29473c';ctx.strokeRect(1,1,254,254);ctx.fillStyle='#b7cebf';ctx.font='14px system-ui';ctx.fillText(text,14,130);tile.dataset.state=state;}
function paintOfflineTile(tile,coords,buffer,zoom){
 if(buffer.byteLength!==131072||!Number.isInteger(zoom)||zoom<10||zoom>coords.z)throw new Error('Invalid map tile');
 const source=document.createElement('canvas');source.width=source.height=256;
 const ctx=source.getContext('2d'),image=ctx.createImageData(256,256),raw=new DataView(buffer);
 for(let i=0;i<65536;i++){const color=raw.getUint16(i*2,true),r=color>>11,g=(color>>5)&63,b=color&31;image.data[i*4]=(r<<3)|(r>>2);image.data[i*4+1]=(g<<2)|(g>>4);image.data[i*4+2]=(b<<3)|(b>>2);image.data[i*4+3]=255;}
 ctx.putImageData(image,0,0);
 const factor=2**(coords.z-zoom),size=256/factor,out=tile.getContext('2d');out.imageSmoothingEnabled=false;
 out.drawImage(source,(coords.x%factor)*size,(coords.y%factor)*size,size,size,0,0,256,256);
 tile.dataset.state=zoom===coords.z?'ready':'coarse';tile.dataset.zoom=String(zoom);
 if(zoom<coords.z){out.fillStyle='rgba(7,16,13,.85)';out.fillRect(4,232,138,20);out.fillStyle='#e8fff5';out.font='12px system-ui';out.fillText(`Coarser map · zoom ${zoom}`,9,246);}
}
async function pumpOfflineTiles(){
 if(offlineBusy)return;const item=offlineQueue.shift();if(!item)return;if(item.cancelled){pumpOfflineTiles();return;}offlineBusy=true;
 const {tile,coords,done,generation}=item,key=`${generation}/${coords.z}/${coords.x}/${coords.y}`;let error=null;
 try{
  let data=offlineCache.get(key);
  if(!data){
   for(let attempt=0;attempt<3;attempt++){
    if(item.cancelled)return;
    const controller=new AbortController();item.controller=controller;const timeout=setTimeout(()=>controller.abort(),8000);
    let response,buffer;try{response=await fetch(`/map/tile?z=${coords.z}&x=${coords.x}&y=${coords.y}`,{signal:controller.signal,cache:'no-store'});if(response.ok)buffer=await response.arrayBuffer();}finally{clearTimeout(timeout);}
    if(item.cancelled)return;
    if(response.status===503&&attempt<2){await new Promise(resolve=>setTimeout(resolve,400*(attempt+1)));continue;}
    if(response.status===404){offlineTileNotice(tile,'No map coverage','missing');return;}
    if(!response.ok)throw new Error(response.status===422?'Unreadable map file':`Map unavailable (${response.status})`);
    const zoom=Number(response.headers.get('X-Tile-Zoom')),actualGeneration=Number(response.headers.get('X-Map-Generation'));
    if(generation!==null&&actualGeneration!==generation)throw new Error('Map storage changed');
    if(buffer.byteLength!==131072||!Number.isInteger(zoom)||zoom<10||zoom>coords.z)throw new Error('Invalid map tile');
    data={buffer,zoom};offlineCache.set(key,data);if(offlineCache.size>24)offlineCache.delete(offlineCache.keys().next().value);break;
   }
  }
  if(!item.cancelled)paintOfflineTile(tile,coords,data.buffer,data.zoom);
 }catch(err){error=err;if(!item.cancelled)offlineTileNotice(tile,err.message==='Unreadable map file'?err.message:'Map unavailable · Refresh maps','error');}
 finally{if(!item.cancelled)done(error,tile);offlineBusy=false;updateOfflineMapMeta();setTimeout(pumpOfflineTiles,20);}
}
function initRealMap(){
 if(realMapReady||!window.L)return;
 leafletMap=L.map('realMap',{zoomControl:true,attributionControl:true,minZoom:10,maxZoom:14});
 const OfflineLayer=L.GridLayer.extend({createTile(coords,done){const tile=document.createElement('canvas');tile.width=tile.height=256;offlineTileNotice(tile,'Loading SD map…','loading');const item={tile,coords,done,generation:offlineGeneration,cancelled:false};tile._request=item;offlineQueue.push(item);setTimeout(pumpOfflineTiles,0);return tile;}});
 offlineLayer=new OfflineLayer({tileSize:256,minZoom:10,maxZoom:14,noWrap:true,keepBuffer:0,updateWhenIdle:true,updateWhenZooming:false,attribution:'SD maps · USGS / &copy; OpenStreetMap contributors'});
 offlineLayer.on('tileunload',event=>{const item=event.tile._request;if(item){item.cancelled=true;if(item.controller)item.controller.abort();}});
 leafletMap.on('moveend zoomend',updateOfflineMapMeta);
 L.control.scale({imperial:true,metric:false}).addTo(leafletMap);realMapReady=true;
}
function cfgValueMatches(e,v){if(e.type==='checkbox')return Boolean(e.checked)===Boolean(v);return String(e.value)===String(v);}
function cfgMaybeHold(e,v){if(!e||v===undefined||v===null)return true;if(e.dataset.dirty==='1'){if(cfgValueMatches(e,v))e.dataset.dirty='0';else return true}return document.activeElement===e;}
function setCfgValue(id,v){const e=document.getElementById(id);if(cfgMaybeHold(e,v))return;if(e.type==='checkbox')e.checked=!!v;else e.value=v;}
function setCfgSelect(id,v){const e=document.getElementById(id);if(cfgMaybeHold(e,v))return;const value=String(v);if(![...e.options].some(o=>o.value===value||o.text===value))e.add(new Option(value,value));e.value=value;}
function applyCurrentConfig(s){document.body.classList.toggle('setup-required',!!s.setupRequired);if(s.setupRequired){openSettingsPanel('cfgInterface');ifaceStatus.textContent='Default login is active. Set a new password before using the rest of the Web UI.';ifaceWebPass.placeholder='Required, 8+ characters';}setCfgValue('ifaceName',s.deviceName);setCfgValue('ifaceHost',s.hostname);setCfgValue('ifaceApSsid',s.apSsid);setCfgValue('ifaceApChannel',s.apChannel);setCfgValue('ifaceWebUser',s.webUser);const c=s.heltecConfig;if(!c)return;if(c.lora&&c.lora.valid){setCfgSelect('hcRegion',c.lora.region);setCfgSelect('hcPreset',c.lora.preset);setCfgValue('hcHop',c.lora.hop);setCfgValue('hcTx',c.lora.txPower)}if(c.serial&&c.serial.valid){setCfgValue('hcSerialEnabled',c.serial.enabled);setCfgValue('hcSerialEcho',c.serial.echo);setCfgValue('hcSerialRxd',c.serial.rxd);setCfgValue('hcSerialTxd',c.serial.txd);setCfgSelect('hcSerialBaud',c.serial.baud);setCfgSelect('hcSerialMode',c.serial.mode);setCfgValue('hcSerialTimeout',c.serial.timeout);setCfgValue('hcSerialOverride',c.serial.override)}if(c.device&&c.device.valid){setCfgSelect('hcRole',c.device.role);setCfgSelect('hcRebroadcast',c.device.rebroadcast);setCfgValue('hcNodeInfo',c.device.nodeInfo);if(c.device.tz)setCfgValue('hcTz',c.device.tz);setCfgValue('hcLedOff',c.device.ledOff);setCfgSelect('hcBuzzer',c.device.buzzer)}if(c.position&&c.position.valid){setCfgValue('hcGpsEnabled',c.position.gpsEnabled);setCfgSelect('hcGpsMode',c.position.gpsMode);setCfgValue('hcFixedPosition',c.position.fixed);setCfgValue('hcSmartPosition',c.position.smart);setCfgValue('hcPositionBroadcast',c.position.broadcast);setCfgValue('hcGpsUpdate',c.position.gpsUpdate);setCfgValue('hcGpsAttempt',c.position.gpsAttempt);setCfgValue('hcSmartMeters',c.position.smartMeters);setCfgValue('hcSmartSecs',c.position.smartSecs)}if(c.power&&c.power.valid){setCfgValue('hcPowerSaving',c.power.saving);setCfgValue('hcShutdown',c.power.shutdown);setCfgValue('hcWaitBt',c.power.waitBt);setCfgValue('hcSds',c.power.sds);setCfgValue('hcLs',c.power.ls);setCfgValue('hcWake',c.power.wake)}if(c.display&&c.display.valid){setCfgValue('hcDisplayScreen',c.display.screen);setCfgValue('hcDisplayCarousel',c.display.carousel);setCfgSelect('hcDisplayUnits',c.display.units);setCfgSelect('hcDisplayOled',c.display.oled);setCfgSelect('hcDisplayMode',c.display.mode);setCfgValue('hcDisplayNorth',c.display.north);setCfgValue('hcDisplayFlip',c.display.flip);setCfgValue('hcDisplayBold',c.display.bold);setCfgValue('hcDisplayWake',c.display.wake);setCfgValue('hcDisplay12h',c.display.clock12);setCfgValue('hcDisplayLongName',c.display.longName);setCfgValue('hcDisplayBubbles',c.display.bubbles)}if(c.network&&c.network.valid){setCfgValue('hcNetWifi',c.network.wifi);setCfgValue('hcNetSsid',c.network.ssid);setCfgValue('hcNetNtp',c.network.ntp);setCfgValue('hcNetSyslog',c.network.syslog);setCfgValue('hcNetEth',c.network.eth);setCfgValue('hcNetIpv6',c.network.ipv6);setCfgValue('hcNetUdp',c.network.udp);const np=document.getElementById('hcNetPsk');if(np&&document.activeElement!==np&&np.dataset.dirty!=='1'){np.value='';np.placeholder=c.network.pskSet?'WiFi password set on radio':'No WiFi password on radio'}const ns=document.getElementById('networkStatus');if(ns)ns.textContent=c.network.pskSet?'Heltec WiFi password is set. Leave blank to keep it.':'No Heltec WiFi password is set.'}if(c.bluetooth&&c.bluetooth.valid){setCfgValue('hcBtEnabled',c.bluetooth.enabled);setCfgSelect('hcBtMode',c.bluetooth.mode);setCfgValue('hcBtPin',c.bluetooth.pin)}const m=c.modules||{},mq=m.mqtt||{};if(mq.valid){setCfgValue('hcMqttEnabled',mq.enabled);setCfgValue('hcMqttAddress',mq.address);setCfgValue('hcMqttRoot',mq.root);setCfgValue('hcMqttUser',mq.username);setCfgValue('hcMqttEncrypt',mq.encryption);setCfgValue('hcMqttJson',mq.json);setCfgValue('hcMqttTls',mq.tls);setCfgValue('hcMqttProxy',mq.proxy);setCfgValue('hcMqttMap',mq.mapReporting);const p=document.getElementById('hcMqttPass');if(p&&document.activeElement!==p&&p.dataset.dirty!=='1'){p.value='';p.placeholder=mq.passwordSet?'Password set on device':'No MQTT password on device'}const ms=document.getElementById('mqttStatus');if(ms)ms.textContent=mq.passwordSet?'MQTT password is set on the device. Leave blank to keep it.':'No MQTT password is set on the device.'}const n=m.externalNotification||{};if(n.valid){setCfgValue('hcNotifyEnabled',n.enabled);setCfgValue('hcNotifyOutput',n.output);setCfgValue('hcNotifyVibra',n.vibra);setCfgValue('hcNotifyBuzzer',n.buzzer);setCfgValue('hcNotifyMs',n.outputMs);setCfgValue('hcNotifyNag',n.nag);setCfgValue('hcNotifyActive',n.active);setCfgValue('hcNotifyMsg',n.alertMessage);setCfgValue('hcNotifyMsgVibra',n.alertMessageVibra);setCfgValue('hcNotifyMsgBuzzer',n.alertMessageBuzzer);setCfgValue('hcNotifyBell',n.alertBell);setCfgValue('hcNotifyBellVibra',n.alertBellVibra);setCfgValue('hcNotifyBellBuzzer',n.alertBellBuzzer);setCfgValue('hcNotifyPwm',n.usePwm);setCfgValue('hcNotifyI2s',n.useI2s)}const t=m.telemetry||{};if(t.valid){setCfgValue('hcTeleDevice',t.deviceEnabled);setCfgValue('hcTeleDeviceSecs',t.deviceInterval);setCfgValue('hcTeleEnv',t.envEnabled);setCfgValue('hcTeleEnvScreen',t.envScreen);setCfgValue('hcTeleEnvF',t.envFahrenheit);setCfgValue('hcTeleEnvSecs',t.envInterval);setCfgValue('hcTeleAir',t.airEnabled);setCfgValue('hcTeleAirScreen',t.airScreen);setCfgValue('hcTeleAirSecs',t.airInterval);setCfgValue('hcTelePower',t.powerEnabled);setCfgValue('hcTelePowerScreen',t.powerScreen);setCfgValue('hcTelePowerSecs',t.powerInterval);setCfgValue('hcTeleHealth',t.healthEnabled);setCfgValue('hcTeleHealthScreen',t.healthScreen);setCfgValue('hcTeleHealthSecs',t.healthInterval)}const sf=m.storeForward||{};if(sf.valid){setCfgValue('hcStoreEnabled',sf.enabled);setCfgValue('hcStoreHeartbeat',sf.heartbeat);setCfgValue('hcStoreServer',sf.server);setCfgValue('hcStoreRecords',sf.records);setCfgValue('hcStoreMax',sf.historyMax);setCfgValue('hcStoreWindow',sf.historyWindow)}const rt=m.rangeTest||{};if(rt.valid){setCfgValue('hcRangeEnabled',rt.enabled);setCfgValue('hcRangeSender',rt.sender);setCfgValue('hcRangeSave',rt.save);setCfgValue('hcRangeClear',rt.clear)}const ni=m.neighborInfo||{};if(ni.valid){setCfgValue('hcNeighborEnabled',ni.enabled);setCfgValue('hcNeighborLora',ni.lora);setCfgValue('hcNeighborSecs',ni.interval)}const cm=m.cannedMessage||{};if(cm.valid){setCfgValue('hcCannedEnabled',cm.enabled);setCfgValue('hcCannedRotary',cm.rotary);setCfgValue('hcCannedUpdown',cm.updown);setCfgValue('hcCannedBell',cm.sendBell);setCfgValue('hcCannedSource',cm.source);setCfgValue('hcCannedPinA',cm.pinA);setCfgValue('hcCannedPinB',cm.pinB);setCfgValue('hcCannedPinPress',cm.pinPress)}const au=m.audio||{};if(au.valid){setCfgValue('hcAudioEnabled',au.enabled);setCfgSelect('hcAudioRate',au.bitrate);setCfgValue('hcAudioPtt',au.ptt);setCfgValue('hcAudioWs',au.ws);setCfgValue('hcAudioSd',au.sd);setCfgValue('hcAudioDin',au.din);setCfgValue('hcAudioSck',au.sck)}const al=m.ambientLighting||{};if(al.valid){setCfgValue('hcAmbientEnabled',al.enabled);setCfgValue('hcAmbientCurrent',al.current);setCfgValue('hcAmbientRed',al.red);setCfgValue('hcAmbientGreen',al.green);setCfgValue('hcAmbientBlue',al.blue)}const ds=m.detectionSensor||{};if(ds.valid){setCfgValue('hcDetectEnabled',ds.enabled);setCfgValue('hcDetectBell',ds.sendBell);setCfgValue('hcDetectPullup',ds.pullup);setCfgValue('hcDetectName',ds.name);setCfgValue('hcDetectPin',ds.pin);setCfgSelect('hcDetectTrigger',ds.trigger);setCfgValue('hcDetectMin',ds.minSecs);setCfgValue('hcDetectState',ds.stateSecs)}const px=m.paxcounter||{};if(px.valid){setCfgValue('hcPaxEnabled',px.enabled);setCfgValue('hcPaxSecs',px.interval);setCfgValue('hcPaxWifi',px.wifi);setCfgValue('hcPaxBle',px.ble)}const sm=m.statusMessage||{};if(sm.valid){setCfgValue('hcStatusMessage',sm.text)}}
let refreshInFlight=null,refreshTimer=null,refreshDelay=1000;
function refresh(){
  if(refreshInFlight)return refreshInFlight;
  clearTimeout(refreshTimer);
  refreshInFlight=refreshStatus().then(()=>{refreshDelay=1000}).catch(err=>{
    ip.textContent='Status unavailable: '+err.message;
    statusFreshness.textContent='Connection interrupted · retrying';
    radioHealth.classList.toggle('warning',true);
    radioHealth.textContent='Device unreachable · displayed readings may be stale';
    refreshDelay=Math.min(refreshDelay*2,15000);
  }).finally(()=>{
    refreshInFlight=null;
    refreshTimer=setTimeout(refresh,refreshDelay);
  });
  return refreshInFlight;
}
async function refreshStatus(){
const controller=new AbortController(),timeout=setTimeout(()=>controller.abort(),8000);
let s;
try{
  const response=await fetch('/status',{signal:controller.signal});
  if(!response.ok)throw new Error('HTTP '+response.status);
  s=await response.json();
}finally{clearTimeout(timeout)}document.title=s.title||'LoRa Interface';appTitle.textContent=s.title||'LoRa Interface';ip.textContent=s.wifiMode+' '+s.ip;statusFreshness.textContent='Live · updated just now';
renderFieldStatus(s);
stats.innerHTML=[['Heltec',s.linkHealth||'waiting'],['Battery',s.fuelGaugePresent?`${s.battery}% · ${s.voltage} V`:'Gauge unavailable'],['Remaining',batteryRuntimeLabel(s)],['Known nodes',s.nodes?s.nodes.length:(s.total||0)]]
.map(x=>`<div class=stat><div class=label>${h(x[0])}</div><div class=value>${h(x[1])}</div></div>`).join('');
firmwareStats.innerHTML=[['Version',s.firmwareVersion],['Source',`${s.gitCommit||'unknown'}${s.sourceDirty?' (modified)':''}`],['Build',`${s.buildDate} ${s.buildTime}`],['Partition',s.runningPartition],['Sketch',formatBytes(Math.ceil((s.sketchSize||0)/1024))],['OTA Space',formatBytes(Math.floor((s.freeSketchSpace||0)/1024))],['AP',s.wifiMode+' '+s.ip]]
.map(x=>`<div class=stat><div class=label>${h(x[0])}</div><div class=value>${h(x[1])}</div></div>`).join('');
renderStatGroup('storageStats',[['Status',s.sdAvailable?'Mounted':s.sdStatus],['Type',s.sdType],['Used',formatBytes(s.sdUsedKb)],['Total',formatBytes(s.sdTotalKb)]]);
renderStatGroup('storageDetailStats',[['Writes',s.sdWrites],['Errors',s.sdErrors],['Busy waits',s.sdLockTimeouts||0],['Append queue',`${s.sdAppendQueueWaiting||0}/${s.sdAppendQueueDepth||0}`],['Append OK',`${s.sdAppendWritten||0}/${s.sdAppendQueued||0}`],['Append drops',s.sdAppendDropped||0],['Map cache',s.mapCacheLoaded?'Loaded':s.mapCacheStatus],['Tile root',s.mapTileRootFound?s.mapTileRoot:'Not found'],['Chat history',s.chatHistoryLoaded?'Loaded':'None yet']]);
const hc=s.heltecConfig||{};
configRef.innerHTML=[['Node',s.myNode],['Name',s.myNodeName],['LoRa',hc.lora&&hc.lora.valid?`${hc.lora.region} ${hc.lora.preset} hop ${hc.lora.hop}`:'not loaded'],['Serial',hc.serial&&hc.serial.valid?`${hc.serial.enabled?'on':'off'} ${hc.serial.mode} ${hc.serial.baud}`:'not loaded'],['Device',hc.device&&hc.device.valid?`${hc.device.role} ${hc.device.rebroadcast}`:'not loaded'],['Position',hc.position&&hc.position.valid?`${hc.position.gpsMode} ${hc.position.broadcast}s`:'not loaded'],['Power',hc.power&&hc.power.valid?`${hc.power.saving?'saving':'normal'} wake ${hc.power.wake}s`:'not loaded'],['Config age',hc.ageSec>=0?hc.ageSec+'s':'none'],['Module age',hc.moduleAgeSec>=0?hc.moduleAgeSec+'s':'none']]
.map(x=>`<div class=stat><div class=label>${h(x[0])}</div><div class=value>${h(x[1])}</div></div>`).join('');
applyCurrentConfig(s);
renderHealth(s);
packetStats.innerHTML=[['Frames',s.frames],['Decode errors',s.errors],['Stream frames',s.streamFrames],['Bad lengths',s.badLengths],['Frame timeouts',s.frameTimeouts||0],['Text',s.textPackets],['Telemetry',s.telemetryPackets],['Positions',s.positionPackets],['Remote positions',s.remotePositionPackets],['Node info',s.nodeInfoPackets],['Config',s.configFrames],['Encrypted',s.encryptedPackets],['Other',s.otherFrames],['RX/TX packets',s.rx+'/'+s.tx],['RX/TX bytes',s.bytes+'/'+s.txBytes]]
.map(x=>`<div class=stat><div class=label>${h(x[0])}</div><div class=value>${h(x[1])}</div></div>`).join('');
currentChannels=s.channels||[];updateChannelPicker();syncSelectedChannelEditor(false);updateChatChannels(s);updateConfigWorkflow(s);
channelRef.innerHTML=(currentChannels.length?currentChannels.map(c=>`<tr onclick="openChannelEditor(${c.index})"><td>${c.index}</td><td>${friendlyRole(c.role)}</td><td>${h(c.name||'-')}</td><td>${c.enabled?'enabled':'disabled'}</td><td>${h(locationLabel(c))}</td><td>${c.uplink?'yes':'-'}</td><td>${c.downlink?'yes':'-'}</td><td>${c.pskSize?'present':'none'}</td></tr>`).join(''):'<tr><td colspan="8">No channel config received yet</td></tr>');
const publicName=channelLabel(0,'primary'),secondaryIdx=secondaryChatIndex(s),secondaryName=channelLabel(secondaryIdx,`channel ${secondaryIdx}`);
updateChannelPairStatus(s,secondaryIdx,secondaryName);
publicChatLog.textContent=s.publicChat||`No ${publicName} chat yet`;familyChatLog.textContent=s.secondaryChat||s.familyChat||`No ${secondaryName} chat yet`;directChatLog.textContent=s.directChat||'No direct messages yet';eventLog.textContent=s.log||'Waiting for radio data';
serialLog.textContent=`RX bytes: ${s.bytes}\nTX bytes: ${s.txBytes}\nLast byte: ${s.lastByte}\nMagic 94/C3: ${s.magic1}/${s.magic2}\nStream frames: ${s.streamFrames}\nBad lengths: ${s.badLengths}\nText/Tel/GPS/Node: ${s.textPackets}/${s.telemetryPackets}/${s.positionPackets}/${s.nodeInfoPackets}\nRemote GPS mapped: ${s.remotePositionPackets}\nConfig/Other/Encrypted: ${s.configFrames}/${s.otherFrames}/${s.encryptedPackets}\nLast port: ${s.lastPort}\nASCII seen: ${s.serialPeek||''}`;
diagSerial.textContent=`Last byte: ${s.lastByte}\nLast port: ${s.lastPort}\nASCII seen: ${s.serialPeek||''}\nLocal GPS bytes: ${s.localGpsBytes}\nGPS sentences with fix: ${s.localGpsSentences}\nGPS checksum failures: ${s.localGpsFailedChecksum}`;
i2cProbeStatus.textContent=`${s.i2cProbeSummary||'Tap Probe I2C Map to scan the bus.'}\n\n${s.i2cProbeDetails||''}`;
renderNodes(s.nodes||[]);
if(currentPage==='map')drawMap(s);
}
function batteryRuntimeLabel(s){if(!s.fuelGaugePresent)return 'Unavailable (no gauge)';if(s.batteryCharging)return 'Charging';const raw=s.batteryRuntimeMinutes;if(!Number.isFinite(raw)||raw<0)return 'Estimating remaining time';const m=raw===0?0:Math.max(5,Math.round(raw/5)*5);const basis={previous:'previous use',gauge:'gauge trend',current:'current use'}[s.batteryRuntimeBasis];return `~${Math.floor(m/60)}h ${m%60}m${basis?' · '+basis:''}`;}
function renderFieldStatus(s){
 renderOfflineCoverage(s);
 const state=s.linkHealth||'waiting';
 radioHealth.textContent=`Heltec: ${state} | last valid response: ${s.lastRadioResponseSec==null?'never':s.lastRadioResponseSec+'s ago'}`;
 radioHealth.classList.toggle('warning',state!=='responsive');
 txHistoryBody.innerHTML=(s.txHistory||[]).map(t=>{
  const id=Number(t.id),valid=Number.isInteger(id)&&id>0&&id<=4294967295;
  const retry=t.canRetry&&valid?`<button onclick="retryMessage(${id})">Retry</button>`:t.retriedAs?'Retried':'—';
  const tone=['acknowledged','mesh acknowledged'].includes(t.status)?'ack':t.status==='failed'?'failed':['queued','sent to radio'].includes(t.status)?'pending':'';
  return `<tr><td>${h(t.destination)}</td><td>${h(t.text)}</td><td><span class="deliveryBadge ${tone}">${h(t.status)}</span>${t.error?'<span class="deliveryMeta">'+h(t.error)+'</span>':''}<span class="deliveryMeta">${h(t.ageSec)}s ago${t.localEcho?' · local echo':''}</span></td><td>${retry}</td></tr>`;
 }).join('')||'<tr><td colspan="4">No messages sent this session</td></tr>';
}
let retryPending=false;
function renderOfflineCoverage(s){
 const shown=Number.isInteger(s.mapDisplayedZoom)?s.mapDisplayedZoom:'none';
 const rows=[['Requested zoom',s.mapRequestedZoom??'unknown'],['Displayed zoom',shown]];
 if(s.mapCoverageChecked){
  const age=Number.isFinite(s.mapCoverageAgeSec)?`${s.mapCoverageAgeSec}s ago`:'earlier';
  mapCoverageStatus.textContent=`Availability around the touchscreen map center, checked ${age}.`;
  for(const level of (Array.isArray(s.mapCoverageLevels)?s.mapCoverageLevels:[]))rows.push([`Zoom ${level.zoom}`,`${level.status} (${level.available}/${level.required})${level.invalid?` · ${level.invalid} unreadable`:''}`]);
 }else mapCoverageStatus.textContent='Open Map on the touchscreen to check available maps in that area.';
 mapCoverageStats.innerHTML=rows.map(x=>`<div class=stat><div class=label>${h(x[0])}</div><div class=value>${h(x[1])}</div></div>`).join('');
}
async function retryMessage(id){
 if(retryPending)return;
 retryPending=true;txRetryStatus.textContent='Queueing retry…';
 const controller=new AbortController(),timeout=setTimeout(()=>controller.abort(),8000);
 try{const r=await fetch('/tx/retry',{method:'POST',signal:controller.signal,headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'id='+encodeURIComponent(id)});const text=await r.text();txRetryStatus.textContent=text||(r.ok?'Retry queued':'Retry unavailable');}
 catch(e){txRetryStatus.textContent='Retry result unknown. Refresh delivery history before trying again.';}
 finally{clearTimeout(timeout);retryPending=false;refresh();}
}
async function send(){const m=msg.value.trim();if(!m)return;chatStatus.textContent='Sending...';const r=await fetch('/send',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'msg='+encodeURIComponent(m)+'&channel='+encodeURIComponent(chatChannel.value)});chatStatus.textContent=r.ok?'Queued on '+selectedChatChannelLabel()+'; see delivery history':((await r.text())||'Send failed');if(r.ok)msg.value='';refresh();}
async function sendDirect(){const to=directTo.value.trim(),m=directMsg.value.trim();if(!to||!m){directStatus.textContent='Enter a node ID and message';return}directStatus.textContent='Sending direct...';const r=await fetch('/send',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'msg='+encodeURIComponent(m)+'&channel=direct&to='+encodeURIComponent(to)});directStatus.textContent=r.ok?'Direct queued; see delivery history':'Direct send failed';if(r.ok)directMsg.value='';refresh();}
async function sendCmd(){const c=cmd.value.trim();if(!c)return;await fetch('/serial_cmd',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'cmd='+encodeURIComponent(c)});cmd.value='';refresh();}
async function mountSd(){await fetch('/sd/mount',{method:'POST'});refresh();}
async function saveSnapshot(){snapshotStatus.textContent='Saving snapshot...';const r=await fetch('/sd/snapshot',{method:'POST'});snapshotStatus.textContent=r.ok?'Snapshot saved to SD':'Snapshot save failed';refresh();}
async function downloadSnapshot(){const s=await (await fetch('/status')).json();const blob=new Blob([JSON.stringify(s,null,2)],{type:'application/json'});const a=document.createElement('a');a.href=URL.createObjectURL(blob);a.download='heltec_status_snapshot.json';a.click();URL.revokeObjectURL(a.href);}
async function testTone(){toneStatus.textContent='Requesting saved message tone...';try{const r=await fetch('/tone/test',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:''});if(!r.ok){toneStatus.textContent=r.status===409?'Sound is busy, muted, or recovering. Try again shortly.':'Tone test failed';return}const s=await r.json();toneStatus.textContent=`Queued message tone: ${s.toneMessage} at ${s.toneVolume}%`;setTimeout(refresh,500);}catch(_){toneStatus.textContent='Device unavailable';}}
async function probeI2cMap(){i2cProbeStatus.textContent='Scanning I2C map...';const r=await fetch('/i2c/probe',{method:'POST'});if(!r.ok){i2cProbeStatus.textContent='I2C probe failed';return}const s=await r.json();i2cProbeStatus.textContent=`${s.summary}\n\n${s.details}`;setTimeout(refresh,400);}
function renderNodes(nodes){currentNodes=Array.isArray(nodes)?nodes.slice():[];updateNodeTable();}
function updateNodeTable(){const q=(nodeSearch.value||'').trim().toLowerCase(),favs=!!nodeFavoritesOnly.checked;let rows=currentNodes.slice().sort((a,b)=>(b.favorite?1:0)-(a.favorite?1:0)||Number(a.age||0)-Number(b.age||0)||String(a.name||a.num).localeCompare(String(b.name||b.num)));if(favs)rows=rows.filter(n=>!!n.favorite);if(q)rows=rows.filter(n=>String(n.name||'').toLowerCase().includes(q)||String(n.num||'').toLowerCase().includes(q));nodeFilterStatus.textContent=`Showing ${rows.length} of ${currentNodes.length} nodes${favs?' | favorites only':''}${q?' | search: '+q:''}`;nodesBody.innerHTML=rows.length?rows.map(n=>{const fav=!!n.favorite,id=String(n.num||'').replace(/[^!0-9a-fA-F]/g,''),gps=n.hasPosition?`${Number(n.lat).toFixed(5)}, ${Number(n.lon).toFixed(5)}`:'-',btn=fav?'Unfavorite':'Favorite';return `<tr class="${fav?'favoriteRow':''}"><td>${h(n.num)}</td><td>${h(n.name||'')}</td><td>${h(n.snr)}</td><td>${h(n.age)}s</td><td>${h(gps)}</td><td><button class="miniAction ${fav?'secondary':''}" onclick="toggleNodeFavorite(event,'${id}',${fav?0:1})">${btn}</button></td></tr>`}).join(''):`<tr><td colspan="6">${currentNodes.length?'No nodes match filter':'No nodes heard yet'}</td></tr>`;}
async function toggleNodeFavorite(e,node,favorite){if(e)e.stopPropagation();nodesStatus.textContent=(favorite?'Adding ':'Removing ')+node+' as favorite on Heltec...';const body=new URLSearchParams({node,favorite:favorite?'1':'0'});const r=await fetch('/nodes/favorite',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body});if(r.ok){nodesStatus.textContent=(favorite?'Favorite':'Favorite removed')+' command sent to Heltec for '+node;setTimeout(refresh,400)}else{nodesStatus.textContent=(await r.text())||'Favorite command failed';}}
async function clearNodes(){nodesStatus.textContent='Clearing node list and Heltec NodeDB...';const r=await fetch('/nodes/clear',{method:'POST'});nodesStatus.textContent=r.ok?'NodeDB reset sent. Waiting for fresh radio packets.':'NodeDB reset failed';refresh();}
async function requestConfig(){if(configWorkflow)configWorkflow.textContent='Requesting config from Heltec...';if(hcStatus)hcStatus.textContent='Requesting config...';const r=await fetch('/config',{method:'POST'});const text=r.ok?'Config request sent. Waiting for Heltec replies.':'Config request failed';if(configWorkflow)configWorkflow.textContent=text;if(hcStatus)hcStatus.textContent=text;setTimeout(refresh,700);}
function setSettingsFeedback(message,owner=settingsPanel){
 settingsActionMessages[owner]=message;
 if(owner===settingsPanel)moduleStatus.textContent=message;
 if(owner==='cfgRadio')hcStatus.textContent=message;
 if(owner==='commands')commandStatus.textContent=message;
}
async function heltecCmd(command,label){
 const owner=currentPage==='config'?settingsPanel:'commands';setSettingsFeedback('Sending...',owner);
 try{const response=await fetch('/serial_cmd',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'cmd='+encodeURIComponent(command)});setSettingsFeedback(response.ok?(label||'Command sent: '+command):'Command failed: '+command,owner);}
 catch(_){setSettingsFeedback('Connection lost. The command result is unknown; refresh before retrying.',owner);}
}
async function heltecAction(url,label){
 const owner=currentPage==='config'?settingsPanel:'commands';setSettingsFeedback('Sending...',owner);
 try{const response=await fetch(url,{method:'POST'});setSettingsFeedback(response.ok?label:'Request failed',owner);setTimeout(refresh,400);}
 catch(_){setSettingsFeedback('Connection lost. The command result is unknown; refresh before retrying.',owner);}
}
async function heltecPost(url,fields,label){
 const owner=settingsPanel;setSettingsFeedback('Sending...',owner);
 try{const response=await fetch(url,{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:new URLSearchParams(fields)});setSettingsFeedback(response.ok?label:'Apply failed',owner);setTimeout(refresh,400);return response.ok;}
 catch(_){setSettingsFeedback('Connection lost. The change result is unknown; refresh before retrying.',owner);return false;}
}
async function heltecPostSave(url,fields,label){
 const owner=settingsPanel;setSettingsFeedback('Applying...',owner);
 try{
  let response=await fetch(url,{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:new URLSearchParams(fields)});
  if(!response.ok){setSettingsFeedback('Apply failed: '+((await response.text())||'Request rejected'),owner);return false;}
  response=await fetch('/heltec/save',{method:'POST'});
  setSettingsFeedback(response.ok?(label||'Saved. Waiting for readback.'):'Applied, but save failed. Refresh settings before retrying.',owner);
  if(response.ok)setTimeout(requestConfig,900);
  return response.ok;
 }catch(_){setSettingsFeedback('Connection lost. The change result is unknown; refresh settings before retrying.',owner);return false;}
}
async function heltecBatch(cmds){for(const c of cmds){await heltecCmd(c);await new Promise(r=>setTimeout(r,180));}}
function applyHeltecLora(){heltecPostSave('/heltec/lora',{region:hcRegion.value,preset:hcPreset.value,hop:hcHop.value,tx:hcTx.value},'Radio settings saved. Waiting for readback.');}
function applyHeltecSerial(){heltecPostSave('/heltec/serial',{enabled:hcSerialEnabled.checked?'1':'0',echo:hcSerialEcho.checked?'1':'0',rxd:hcSerialRxd.value,txd:hcSerialTxd.value,baud:hcSerialBaud.value,mode:hcSerialMode.value,timeout:hcSerialTimeout.value,override:hcSerialOverride.checked?'1':'0'},'Serial module saved. Waiting for readback.');}
function applyHeltecName(){const n=hcName.value.trim();if(n)heltecPost('/heltec/name',{name:n,short:hcShortName.value.trim()},'Name update sent');}
function applyHeltecTime(){const z=hcTz.value.trim();if(z)heltecPost('/heltec/timezone',{tz:z},'Timezone update sent');}
async function applyInterfaceSettings(){let ch=parseInt(ifaceApChannel.value||'6',10);if(isNaN(ch)||ch<1)ch=1;if(ch>13)ch=13;ifaceApChannel.value=ch;const pw=ifaceWebPass.value;if(document.body.classList.contains('setup-required')&&pw.length<8){ifaceStatus.textContent='Enter a new password with at least 8 characters.';return}if(pw&&pw.length<8){ifaceStatus.textContent='Password must be at least 8 characters.';return}ifaceStatus.textContent='Saving interface settings...';const body=new URLSearchParams({deviceName:ifaceName.value.trim(),hostname:ifaceHost.value.trim(),apSsid:ifaceApSsid.value.trim(),apChannel:ch,webUser:ifaceWebUser.value.trim(),webPass:pw});const r=await fetch('/interface',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body});const text=await r.text();ifaceStatus.textContent=r.ok?'Interface settings saved. Reconnect if WiFi or login changed.':(text||'Interface settings failed');ifaceWebPass.value='';setTimeout(refresh,700);}
function applyHeltecDevice(){return heltecPostSave('/heltec/device',{role:hcRole.value,rebroadcast:hcRebroadcast.value,nodeInfo:hcNodeInfo.value,tz:hcTz.value.trim(),ledOff:hcLedOff.checked?'1':'0',buzzer:hcBuzzer.value},'Device behavior saved. Waiting for readback.');}
function applyHeltecPosition(){heltecPostSave('/heltec/position',{gpsEnabled:hcGpsEnabled.checked?'1':'0',gpsMode:hcGpsMode.value,fixed:hcFixedPosition.checked?'1':'0',smart:hcSmartPosition.checked?'1':'0',broadcast:hcPositionBroadcast.value,gpsUpdate:hcGpsUpdate.value,gpsAttempt:hcGpsAttempt.value,smartMeters:hcSmartMeters.value,smartSecs:hcSmartSecs.value},'Position config saved. Waiting for readback.');}
function applyHeltecPower(){heltecPostSave('/heltec/power',{saving:hcPowerSaving.checked?'1':'0',shutdown:hcShutdown.value,waitBt:hcWaitBt.value,sds:hcSds.value,ls:hcLs.value,wake:hcWake.value},'Power config saved. Waiting for readback.');}
function applyDisplay(){heltecPostSave('/heltec/display',{screen:hcDisplayScreen.value,carousel:hcDisplayCarousel.value,units:hcDisplayUnits.value,oled:hcDisplayOled.value,mode:hcDisplayMode.value,north:b(hcDisplayNorth),flip:b(hcDisplayFlip),bold:b(hcDisplayBold),wake:b(hcDisplayWake),clock12:b(hcDisplay12h),longName:b(hcDisplayLongName),bubbles:b(hcDisplayBubbles)},'Display saved. Waiting for readback.');}
function applyNetwork(){heltecPostSave('/heltec/network',{wifi:b(hcNetWifi),ssid:hcNetSsid.value.trim(),psk:hcNetPsk.value,ntp:hcNetNtp.value.trim(),syslog:hcNetSyslog.value.trim(),eth:b(hcNetEth),ipv6:b(hcNetIpv6),udp:b(hcNetUdp)},'Heltec network saved. Waiting for readback.');hcNetPsk.value='';hcNetPsk.dataset.dirty='0';}
function applyBluetooth(){heltecPostSave('/heltec/bluetooth',{enabled:b(hcBtEnabled),mode:hcBtMode.value,pin:hcBtPin.value},'Bluetooth saved. Waiting for readback.');}
function applyModule(section,fields,label){fields.section=section;return heltecPostSave('/heltec/module',fields,label);}
function b(e){return e.checked?'1':'0';}
function applyMqtt(){applyModule('mqtt',{enabled:b(hcMqttEnabled),address:hcMqttAddress.value.trim(),root:hcMqttRoot.value.trim(),username:hcMqttUser.value.trim(),password:hcMqttPass.value,encryption:b(hcMqttEncrypt),json:b(hcMqttJson),tls:b(hcMqttTls),proxy:b(hcMqttProxy),mapReporting:b(hcMqttMap)},'MQTT saved. Waiting for readback.');hcMqttPass.value='';hcMqttPass.dataset.dirty='0';}
function applyNotify(){applyModule('external_notification',{enabled:b(hcNotifyEnabled),output:hcNotifyOutput.value,vibra:hcNotifyVibra.value,buzzer:hcNotifyBuzzer.value,outputMs:hcNotifyMs.value,nag:hcNotifyNag.value,active:b(hcNotifyActive),alertMessage:b(hcNotifyMsg),alertMessageVibra:b(hcNotifyMsgVibra),alertMessageBuzzer:b(hcNotifyMsgBuzzer),alertBell:b(hcNotifyBell),alertBellVibra:b(hcNotifyBellVibra),alertBellBuzzer:b(hcNotifyBellBuzzer),usePwm:b(hcNotifyPwm),useI2s:b(hcNotifyI2s)},'Notification saved. Waiting for readback.');}
function applyTelemetry(){applyModule('telemetry',{deviceEnabled:b(hcTeleDevice),deviceInterval:hcTeleDeviceSecs.value,envEnabled:b(hcTeleEnv),envScreen:b(hcTeleEnvScreen),envFahrenheit:b(hcTeleEnvF),envInterval:hcTeleEnvSecs.value,airEnabled:b(hcTeleAir),airScreen:b(hcTeleAirScreen),airInterval:hcTeleAirSecs.value,powerEnabled:b(hcTelePower),powerScreen:b(hcTelePowerScreen),powerInterval:hcTelePowerSecs.value,healthEnabled:b(hcTeleHealth),healthScreen:b(hcTeleHealthScreen),healthInterval:hcTeleHealthSecs.value},'Telemetry saved. Waiting for readback.');}
function applyStoreForward(){applyModule('store_forward',{enabled:b(hcStoreEnabled),heartbeat:b(hcStoreHeartbeat),server:b(hcStoreServer),records:hcStoreRecords.value,historyMax:hcStoreMax.value,historyWindow:hcStoreWindow.value},'Store and forward saved. Waiting for readback.');}
function applyRangeTest(){applyModule('range_test',{enabled:b(hcRangeEnabled),sender:hcRangeSender.value,save:b(hcRangeSave),clear:b(hcRangeClear)},'Range test saved. Waiting for readback.');}
function applyNeighbor(){applyModule('neighbor_info',{enabled:b(hcNeighborEnabled),interval:hcNeighborSecs.value,lora:b(hcNeighborLora)},'Neighbor info saved. Waiting for readback.');}
function applyCanned(){applyModule('canned_message',{enabled:b(hcCannedEnabled),rotary:b(hcCannedRotary),updown:b(hcCannedUpdown),sendBell:b(hcCannedBell),source:hcCannedSource.value.trim(),pinA:hcCannedPinA.value,pinB:hcCannedPinB.value,pinPress:hcCannedPinPress.value,cw:17,ccw:18,press:10},'Canned message saved. Waiting for readback.');}
function applyAudio(){applyModule('audio',{enabled:b(hcAudioEnabled),bitrate:hcAudioRate.value,ptt:hcAudioPtt.value,ws:hcAudioWs.value,sd:hcAudioSd.value,din:hcAudioDin.value,sck:hcAudioSck.value},'Audio saved. Waiting for readback.');}
function applyAmbient(){applyModule('ambient_lighting',{enabled:b(hcAmbientEnabled),current:hcAmbientCurrent.value,red:hcAmbientRed.value,green:hcAmbientGreen.value,blue:hcAmbientBlue.value},'Ambient lighting saved. Waiting for readback.');}
function applyDetection(){applyModule('detection_sensor',{enabled:b(hcDetectEnabled),sendBell:b(hcDetectBell),pullup:b(hcDetectPullup),name:hcDetectName.value.trim(),pin:hcDetectPin.value,trigger:hcDetectTrigger.value,minSecs:hcDetectMin.value,stateSecs:hcDetectState.value},'Detection sensor saved. Waiting for readback.');}
function applyPax(){applyModule('paxcounter',{enabled:b(hcPaxEnabled),interval:hcPaxSecs.value,wifi:hcPaxWifi.value,ble:hcPaxBle.value},'Paxcounter saved. Waiting for readback.');}
function applyStatusMessage(){applyModule('status_message',{text:hcStatusMessage.value.trim()},'Status message saved. Waiting for readback.');}
function friendlyRole(r){return r==='PRIMARY'?'primary':r==='SECONDARY'?'secondary':r==='DISABLED'?'disabled':(r||'-').toLowerCase();}
function locationLabel(c){if(!c||!c.enabled)return'-';if(!c.hasPositionPrecision)return'default';const p=Number(c.positionPrecision||0);if(p<=0)return'hidden';if(p>=32)return'exact';return'approx';}
function channelByIndex(i){return currentChannels.find(c=>Number(c.index)===Number(i));}
function channelLabel(i,fallback){const c=channelByIndex(i);return c&&c.name?c.name:fallback;}
function secondaryChatIndex(s){if(s&&Number(s.secondaryChannel)>=0)return Number(s.secondaryChannel);if(s&&Number(s.privateChannel)>=0)return Number(s.privateChannel);const c=currentChannels.find(x=>x.enabled&&x.role==='SECONDARY');return c?Number(c.index):1;}
function privateChatIndex(s){return secondaryChatIndex(s);}
function selectedChatChannelLabel(){return chatChannel.options[chatChannel.selectedIndex]?chatChannel.options[chatChannel.selectedIndex].text:'selected channel';}
function updateChatStatus(){chatStatus.textContent='Sending on '+selectedChatChannelLabel()+'.';}
function updateChannelPairStatus(s,i,name){const c=channelByIndex(i);if(!channelPairStatus)return;if(c&&c.enabled&&c.role==='SECONDARY'&&Number(c.pskSize||0)>0)channelPairStatus.textContent=`${name} is ready: devices with this channel name and PSK can communicate on channel ${i}.`;else if(c&&c.enabled&&c.role==='SECONDARY')channelPairStatus.textContent=`${name} is enabled on channel ${i}, but no PSK is reported.`;else channelPairStatus.textContent='No enabled secondary channel loaded yet. Refresh config after setting up the other device.';}
function updateChatChannels(s){const selected=chatChannel.value;const enabled=currentChannels.filter(c=>c.enabled);chatChannel.innerHTML='';if(enabled.length){enabled.forEach(c=>chatChannel.add(new Option(`${c.name||('channel '+c.index)} (${friendlyRole(c.role)} ${c.index})`,`ch:${c.index}`)));}else{chatChannel.add(new Option('primary (0)','ch:0'));chatChannel.add(new Option('channel 1','ch:1'));}if([...chatChannel.options].some(o=>o.value===selected))chatChannel.value=selected;const p=channelLabel(0,'primary'),si=secondaryChatIndex(s),sn=channelLabel(si,`channel ${si}`);chatPublicTitle.textContent=p+' Chat';chatFamilyTitle.textContent=sn+' Chat';updateChatStatus();}
function updateConfigWorkflow(s){const hc=s.heltecConfig||{},mods=hc.modules||{};const configAge=hc.ageSec>=0?hc.ageSec+'s old':'not loaded';const moduleAge=hc.moduleAgeSec>=0?hc.moduleAgeSec+'s old':'not loaded';const loadedMods=Object.values(mods).filter(x=>x&&x.valid).length;configWorkflow.textContent=`Heltec settings: ${configAge} · ${currentChannels.length} channels · ${loadedMods} modules. Apply saves settings and requests readback.`;configFlowLine.innerHTML=`<span>Radio ${hc.lora&&hc.lora.valid?'loaded':'needed'}</span><span>Modules ${loadedMods||'needed'}</span><span>Channels ${currentChannels.length||'needed'}</span>`;}
function pskLabel(c){if(!c)return'PSK status unknown until channel config is loaded.';return Number(c.pskSize||0)>0?`PSK present (${c.pskSize} bytes). Leave blank to keep it.`:'No PSK key reported for this channel.';}
function pskPlaceholder(c){if(!c)return'PSK status unknown until channel config is loaded';return Number(c.pskSize||0)>0?`PSK present (${c.pskSize} bytes)`:'No PSK key on device';}
function channelTitle(i){const c=currentChannels.find(x=>Number(x.index)===Number(i));if(!c)return i==0?'0 - Primary':i==1?'1 - Secondary':String(i);return `${c.index} - ${c.name||friendlyRole(c.role)} (${c.enabled?friendlyRole(c.role):'disabled'})`;}
function updateChannelPicker(){const active=hcChanIndex.value||'0';hcChanIndex.innerHTML='';for(let i=0;i<8;i++)hcChanIndex.add(new Option(channelTitle(i),String(i)));hcChanIndex.value=active;}
function setEditorValue(e,v,force){if(!e||(!force&&document.activeElement===e))return;if(e.type==='checkbox')e.checked=!!v;else e.value=v;}
function syncChannelEditor(c,i,force){setEditorValue(hcChanRole,c?(c.role||'DISABLED'):(i===0?'PRIMARY':'SECONDARY'),force);setEditorValue(hcChanName,c?(c.name||''):(i===0?'LongFast':''),force);if(force)hcChanPsk.value='';hcChanPsk.placeholder=pskPlaceholder(c);setEditorValue(hcChanLocation,c&&c.hasPositionPrecision?String(c.positionPrecision<=0?0:c.positionPrecision>=32?32:16):'32',force);setEditorValue(hcChanUplink,!!(c&&c.uplink),force);setEditorValue(hcChanDownlink,!!(c&&c.downlink),force);hcChanPskStatus.textContent=pskLabel(c);}
function syncSelectedChannelEditor(force){if(channelEditorDirty&&!force)return;const i=channelIndex();const c=currentChannels.find(x=>Number(x.index)===i);syncChannelEditor(c,i,force);}
function loadChannelEditor(index){const i=channelIndex(index);const c=currentChannels.find(x=>Number(x.index)===i);hcChanIndex.value=String(i);hcChanPreset.value='';channelEditorDirty=false;syncChannelEditor(c,i,true);channelHelp.textContent=c?`Loaded channel ${i} from device. ${pskLabel(c)}`:`Channel ${i} is not loaded yet. Refresh from Heltec if you expect it to exist.`;}
function openChannelEditor(index){showPage('config');loadChannelEditor(index);setTimeout(()=>jumpToConfig('cfgChannel'),30);}
function markChannelEditorDirty(){channelEditorDirty=true;}
function applyChannelPreset(){const p=hcChanPreset.value;if(!p)return;channelEditorDirty=true;if(p==='primary'){hcChanIndex.value='0';hcChanRole.value='PRIMARY';hcChanName.value=hcChanName.value||'LongFast';hcChanLocation.value='32';hcChanUplink.checked=false;hcChanDownlink.checked=false;channelHelp.textContent='Primary is the main radio channel. Apply, test, then Save To Heltec.';}else if(p==='secondary'){if(hcChanIndex.value==='0')hcChanIndex.value='1';hcChanRole.value='SECONDARY';hcChanLocation.value='0';hcChanUplink.checked=false;hcChanDownlink.checked=false;channelHelp.textContent='Secondary channels are soft-named by the device. Use the same name and PSK on each node that should communicate here.';}else if(p==='disable'){hcChanRole.value='DISABLED';channelHelp.textContent='Disable will turn off the selected channel after Apply Channel Changes.';}}
function channelIndex(v){let i=parseInt((v!==undefined?v:hcChanIndex.value)||'0',10);if(isNaN(i)||i<0)i=0;if(i>7)i=7;if(v===undefined)hcChanIndex.value=String(i);return i;}
function applyHeltecChannel(){const i=channelIndex();const role=hcChanRole.value;if(role==='PRIMARY'&&i!==0&&!confirm('Make channel '+i+' primary? Meshtastic only uses one primary channel.'))return;heltecPost('/heltec/channel',{index:i,role,name:hcChanName.value.trim(),psk:hcChanPsk.value.trim(),uplink:hcChanUplink.checked?'1':'0',downlink:hcChanDownlink.checked?'1':'0',positionPrecision:hcChanLocation.value},'Channel settings sent. Use Save To Heltec after testing.');channelHelp.textContent='Channel settings sent. Use Save To Heltec after testing.';}
function disableHeltecChannel(){channelEditorDirty=true;hcChanRole.value='DISABLED';heltecPost('/heltec/channel',{index:channelIndex(),role:'DISABLED'},'Disable channel sent. Use Save To Heltec after testing.');channelHelp.textContent='Disable command sent. Use Save To Heltec after testing.';}
function formatBytes(kb){if(!kb)return'0 KB';return kb>=1024?Math.ceil(kb/1024)+' MB':kb+' KB';}
function drawMap(s){initRealMap();let pts=s.nodes.filter(n=>n.hasPosition);if(s.gpsValid){const live={num:s.myNode,name:(s.myNodeName||'Me')+' live',snr:0,age:0,hasPosition:true,lat:s.gpsLat,lon:s.gpsLon,alt:0,positionAge:0};const i=pts.findIndex(n=>n.num===s.myNode);if(i>=0)pts[i]=live;else pts=[live,...pts]}if(realMapReady){realMap.classList.remove('hidden');mapFallback.classList.add('hidden');drawRealMap(s,pts);return}realMap.classList.add('hidden');mapFallback.classList.remove('hidden');drawFallbackMap(s,pts);}
function drawRealMap(s,pts){
 offlineStatus=s;offlinePoints=pts;
 const generation=Number.isInteger(s.mapTileStorageGeneration)?s.mapTileStorageGeneration:null;
 if(generation!==offlineGeneration){offlineGeneration=generation;reloadOfflineMap();}
 if(!offlineCentered){const center=offlineMapCenter(s,pts);if(!center){mapMeta.textContent='Waiting for GPS or a saved location';return;}leafletMap.setView(center,14);offlineCentered=true;offlineLayer.addTo(leafletMap);}
 const seen={};
 for(const p of pts){seen[p.num]=true;const html=`<b>${h(p.name||p.num)}</b><br>${h(p.num)}<br>${p.lat.toFixed(6)}, ${p.lon.toFixed(6)}<br>Alt ${h(p.alt)} m<br>${h(p.positionAge)}s old`;if(!markers[p.num])markers[p.num]=L.circleMarker([p.lat,p.lon],{radius:7,color:p.num===s.myNode?'#3b9dff':'#85bfff',weight:2,fillColor:p.num===s.myNode?'#3b9dff':'#85bfff',fillOpacity:.85}).addTo(leafletMap);else markers[p.num].setLatLng([p.lat,p.lon]);markers[p.num].setStyle({color:p.num===s.myNode?'#3b9dff':'#85bfff',fillColor:p.num===s.myNode?'#3b9dff':'#85bfff'});markers[p.num].bindPopup(html);}
 for(const id in markers){if(!seen[id]){leafletMap.removeLayer(markers[id]);delete markers[id];}}
 updateOfflineMapMeta();
}
function drawFallbackMap(s,pts){const c=mapFallback,ctx=c.getContext('2d');ctx.clearRect(0,0,c.width,c.height);ctx.fillStyle='#09182a';ctx.fillRect(0,0,c.width,c.height);ctx.strokeStyle='#1f3d35';ctx.lineWidth=1;for(let x=40;x<c.width;x+=80){ctx.beginPath();ctx.moveTo(x,0);ctx.lineTo(x,c.height);ctx.stroke()}for(let y=40;y<c.height;y+=80){ctx.beginPath();ctx.moveTo(0,y);ctx.lineTo(c.width,y);ctx.stroke()}if(!pts.length){ctx.fillStyle='#8ab7a6';ctx.font='16px system-ui';ctx.fillText('Waiting for position packets',24,40);mapMeta.textContent='No node positions yet';return}let minLat=Math.min(...pts.map(p=>p.lat)),maxLat=Math.max(...pts.map(p=>p.lat)),minLon=Math.min(...pts.map(p=>p.lon)),maxLon=Math.max(...pts.map(p=>p.lon));let latSpan=Math.max(maxLat-minLat,0.000001),lonSpan=Math.max(maxLon-minLon,0.000001),pad=28;for(const p of pts){let x=pad+(p.lon-minLon)*(c.width-pad*2)/lonSpan,y=pad+(maxLat-p.lat)*(c.height-pad*2)/latSpan;ctx.fillStyle=p.num===s.myNode?'#3b9dff':'#85bfff';ctx.beginPath();ctx.arc(x,y,6,0,Math.PI*2);ctx.fill();ctx.fillStyle='#e8fff5';ctx.font='12px system-ui';ctx.fillText(p.name||p.num,x+9,y+4)}mapMeta.textContent=`Offline plot | ${pts.length} positioned node${pts.length===1?'':'s'} | center ${((minLat+maxLat)/2).toFixed(5)}, ${((minLon+maxLon)/2).toFixed(5)}`;}
[hcChanRole,hcChanName,hcChanPsk,hcChanLocation,hcChanUplink,hcChanDownlink].forEach(e=>{e.addEventListener('input',markChannelEditorDirty);e.addEventListener('change',markChannelEditorDirty);});
document.querySelectorAll('#config input,#config select').forEach(e=>{e.addEventListener('input',()=>e.dataset.dirty='1');e.addEventListener('change',()=>e.dataset.dirty='1')});
nodeSearch.addEventListener('input',updateNodeTable);
nodeFavoritesOnly.addEventListener('change',updateNodeTable);
chatChannel.addEventListener('change',updateChatStatus);
firmwareForm.onsubmit=async e=>{e.preventDefault();const f=firmwareFile.files[0];if(!f){firmwareStatus.textContent='Choose a firmware .bin first';return}if(!f.name.endsWith('.bin')){firmwareStatus.textContent='Firmware file should be a .bin';return}firmwareStatus.textContent='Uploading firmware. Do not power off.';const body=new FormData();body.append('firmware',f,f.name);try{const r=await fetch('/firmware',{method:'POST',body});const text=await r.text();firmwareStatus.textContent=r.ok?text:(text||'Firmware upload failed');if(r.ok)setTimeout(()=>{firmwareStatus.textContent='Rebooting. Reconnect to WiFi, then refresh this page.'},900)}catch(err){firmwareStatus.textContent='Upload interrupted. Reconnect and check the device.'}};
refresh();
</script></body></html>
)HTML";
