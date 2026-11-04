const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const path = require('node:path');
const crypto = require('node:crypto');
const zlib = require('node:zlib');

const html = fs.readFileSync(path.join(__dirname, '../src/WebUi.h'), 'utf8');
const script = html.split('<script>')[1].split('</script>')[0];
// Compile the entire UI script, including handlers that are not exercised below.
new vm.Script(script);

function element() {
  return {
    value: '', textContent: '', innerHTML: '', dataset: {}, options: [],
    classList: { toggle() {}, contains() { return false; } },
    add(option) { this.options.push(option); }, addEventListener() {},
    replaceChildren() {}, setAttribute() {},
  };
}

const elements = {};
for (const match of html.matchAll(/\bid="([^"]+)"/g)) elements[match[1]] = element();
const timers = new Map();
let timerId = 0;
const context = vm.createContext({
  ...elements, console, AbortController, URLSearchParams,
  document: {
    addEventListener() {}, querySelectorAll() { return []; },
    getElementById(id) { return elements[id] || null; },
    body: { classList: { toggle() {}, contains() { return false; } } },
  },
  window: {},
  Option: function(text, value) { this.text = text; this.value = value; },
  setTimeout(fn, delay) { const id = ++timerId; timers.set(id, { fn, delay }); return id; },
  clearTimeout(id) { timers.delete(id); },
  fetch: async () => { throw new Error('offline'); },
});
// Do not start polling while installing the script in the fake browser.
vm.runInContext(script.replace(/\nrefresh\(\);\s*$/, '\n'), context);

async function testRendering() {
  const attack = '<img src=x onerror="alert(1)">';
  const status = {
    title: attack, myNode: '!00000001', myNodeName: attack,
    wifiMode: 'AP', ip: '192.168.4.1', gpsValid: false,
    heltecConfig: {}, channels: [{ index: 0, name: attack, role: 'PRIMARY', enabled: true }],
    nodes: [], battery: 50, voltage: 3.8,
    linkHealth: 'stale', lastRadioResponseSec: 95,
    fuelGaugePresent: false, batteryMeasurementAvailable: true, batteryChargingStatus: "charging", batteryRuntimeMinutes: 125,
    txHistory: [{ id: 123, destination: attack, text: attack, status: 'failed', error: attack, ageSec: 20, canRetry: true },
                { id: attack, destination: 'test', text: 'test', status: 'acknowledged', canRetry: true }],
  };
  context.fetch = async () => ({ ok: true, json: async () => status });
  await vm.runInContext('refresh()', context);
  assert(elements.configRef.innerHTML.includes('&lt;img'));
  assert(elements.channelRef.innerHTML.includes('&lt;img'));
  for (const id of ['stats', 'firmwareStats', 'storageStats', 'configRef', 'diagStats', 'packetStats', 'channelRef', 'txHistoryBody']) {
    assert(!elements[id].innerHTML.includes('<img'), `${id} must escape dynamic HTML`);
  }
  assert(elements.stats.innerHTML.includes('~'), 'Voltage percentage must be marked as estimated');
  assert(!elements.stats.innerHTML.includes('Charger'), 'No charger measurement is installed');
  assert.equal(elements.appTitle.textContent, attack);
  assert.equal(elements.radioHealth.textContent, 'Heltec: stale | last valid response: 95s ago');
  assert(elements.txHistoryBody.innerHTML.includes('retryMessage(123)'));
  assert(elements.txHistoryBody.innerHTML.includes('deliveryBadge failed'));
  assert.equal(elements.statusFreshness.textContent, 'Live · updated just now');
  assert.equal((elements.txHistoryBody.innerHTML.match(/retryMessage/g)||[]).length, 1);
  assert.equal(vm.runInContext('batteryRuntimeLabel({batteryMeasurementAvailable:true,batteryCharging:true})', context), 'Unavailable (voltage monitor)');
  for (const [direction,label] of [['rising','+'],['falling','-'],['steady','='],['unavailable','Unavailable']]) {
    context.batteryDirection=direction;
    assert.equal(vm.runInContext('batteryTrendLabel({batteryGaugeTrend:batteryDirection})',context),label);
  }
  assert.equal(vm.runInContext('refreshDelay', context), 1000);
}

async function testSingleRequestAndRetries() {
  let calls = 0, rejectPending;
  context.fetch = () => { calls++; return new Promise((resolve, reject) => { rejectPending = reject; }); };
  const first = vm.runInContext('refresh()', context);
  const second = vm.runInContext('refresh()', context);
  assert.equal(first, second, 'simultaneous refreshes must share one request');
  assert.equal(calls, 1);
  assert.equal(timers.size, 1, 'only the request timeout runs while a request is pending');
  rejectPending(new Error('connection lost'));
  await first;
  assert.equal(elements.ip.textContent, 'Status unavailable: connection lost');
  assert(elements.radioHealth.textContent.includes('readings may be stale'));
  assert(elements.statusFreshness.textContent.includes('retrying'));
  assert.equal(vm.runInContext('refreshDelay', context), 2000);
  assert.equal(timers.size, 1, 'one retry must be scheduled after completion');
  context.fetch = async () => ({ ok: false, status: 401 });
  await vm.runInContext('refresh()', context);
  assert.equal(elements.ip.textContent, 'Status unavailable: HTTP 401');
  assert.equal(vm.runInContext('refreshDelay', context), 4000);
  for (let i = 0; i < 5; ++i) await vm.runInContext('refresh()', context);
  assert.equal(vm.runInContext('refreshDelay', context), 15000);
  await testRendering();
}

async function testRequestTimeout() {
  context.fetch = (url, options) => new Promise((resolve, reject) => {
    options.signal.addEventListener('abort', () => reject(new Error('request timed out')));
  });
  const pending = vm.runInContext('refresh()', context);
  const timeout = [...timers.values()].find(timer => timer.delay === 8000);
  assert(timeout, 'stalled status requests must have a deadline');
  timeout.fn();
  await pending;
  assert.equal(elements.ip.textContent, 'Status unavailable: request timed out');
  assert.equal(timers.size, 1);
}

async function testRetryRequests() {
  let calls = 0, finish;
  context.fetch = async (url, options) => {
    if (url === '/tx/retry') {
      calls++;
      assert.equal(options.method, 'POST');
      assert.equal(options.body, 'id=123');
      return new Promise(resolve => { finish = resolve; });
    }
    throw new Error('offline');
  };
  const first = vm.runInContext('retryMessage(123)', context);
  await vm.runInContext('retryMessage(123)', context);
  assert.equal(calls, 1, 'Repeated taps must not send duplicate retries');
  finish({ok:false,text:async ()=>'Retry unavailable'});
  await first;
  assert.equal(elements.txRetryStatus.textContent, 'Retry unavailable');
  context.fetch = async () => { throw new Error('offline'); };
  await vm.runInContext('retryMessage(123)', context);
  assert(elements.txRetryStatus.textContent.includes('result unknown'));
}

async function testSerialPinSettings(){
  vm.runInContext("settingsPanel='cfgSerial'",context);
  for(const id of ['hcSerialRxd','hcSerialTxd']){
    const tag=html.match(new RegExp('<input[^>]*id="'+id+'"[^>]*>'))[0];
    assert(!/\bvalue=/.test(tag),'Unknown Heltec pins must not default to a specific pair');
    elements[id].value='';elements[id].dataset.dirty='0';
  }
  vm.runInContext('applyCurrentConfig({heltecConfig:{serial:{valid:false,rxd:0,txd:0}}})',context);
  assert.equal(elements.hcSerialRxd.value,'');
  assert.equal(elements.hcSerialTxd.value,'');
  let requests=[];
  context.fetch=async(url,options)=>{requests.push({url,options});return {ok:true};};
  for(const [rx,tx] of [['',''],['47',''],['','48'],['49','48'],['47','-1'],['47.5','48']]){
    elements.hcSerialRxd.value=rx;elements.hcSerialTxd.value=tx;
    assert.equal(await vm.runInContext('applyHeltecSerial()',context),false);
    assert.equal(requests.length,0,'Invalid or missing pins must not write or save radio configuration');
  }
  vm.runInContext('applyCurrentConfig({heltecConfig:{serial:{valid:true,rxd:47,txd:48}}})',context);
  assert.equal(String(elements.hcSerialRxd.value),'47');
  assert.equal(String(elements.hcSerialTxd.value),'48');
  for(const [rx,tx] of [['39','38'],['47','48']]){
    elements.hcSerialRxd.value=rx;elements.hcSerialTxd.value=tx;
    elements.hcSerialRxd.dataset.dirty='1';elements.hcSerialTxd.dataset.dirty='1';
    vm.runInContext('applyCurrentConfig({heltecConfig:{serial:{valid:true,rxd:38,txd:39}}})',context);
    assert.equal(elements.hcSerialRxd.value,rx,'Polling must preserve edited RX');
    assert.equal(elements.hcSerialTxd.value,tx,'Polling must preserve edited TX');
    requests=[];
    assert.equal(await vm.runInContext('applyHeltecSerial()',context),true);
    assert.deepEqual(requests.map(r=>r.url),['/heltec/serial','/heltec/save']);
    const body=new URLSearchParams(requests[0].options.body);
    assert.equal(body.get('rxd'),rx);assert.equal(body.get('txd'),tx);
  }
  vm.runInContext('applyCurrentConfig({heltecConfig:{serial:{valid:true,rxd:47,txd:48}}})',context);
  assert.equal(elements.hcSerialRxd.dataset.dirty,'0','Matching radio readback clears the saved RX draft');
  assert.equal(elements.hcSerialTxd.dataset.dirty,'0','Matching radio readback clears the saved TX draft');
}

async function testBatteryCycleControls(){
  let calls=0,finish;
  context.fetch=(url,options)=>{
    calls++;
    assert.equal(url,'/battery/cycle');
    assert.equal(options.method,'POST');
    assert.equal(new URLSearchParams(options.body).get('action'),'rotation-off');
    return new Promise(resolve=>{finish=resolve;});
  };
  const first=vm.runInContext("batteryCycleAction('rotation-off')",context);
  await vm.runInContext("batteryCycleAction('rotation-off')",context);
  assert.equal(calls,1,'Duplicate retention changes must share one request');
  finish({ok:true,text:async()=> 'Continuous recording to SD'});
  await first;
  assert.equal(elements.batteryCycleStatus.textContent,'Continuous recording to SD');
  context.fetch=async()=>({ok:false,text:async()=> 'Setting not saved'});
  await vm.runInContext("batteryCycleAction('rotation-on')",context);
  assert.equal(elements.batteryCycleStatus.textContent,'Action failed: Setting not saved');
  context.fetch=async()=>{throw new Error('offline');};
  await vm.runInContext("batteryCycleAction('rotation-on')",context);
  assert(elements.batteryCycleStatus.textContent.includes('result unknown'));
}

function testChatColorsAndBatterySymbols(){
  context.chatFixture='[03:00] [me] Mine #FF0000 <img src=x>\n[03:01] [from me] Remote\n  [fake] [me] quoted\n';
  vm.runInContext('renderChatLog(publicChatLog,chatFixture)',context);
  const rendered=elements.publicChatLog.innerHTML;
  assert(rendered.includes('class="chatMine"'));
  assert(rendered.includes('class="chatOther"'));
  assert(rendered.includes('&lt;img'));
  assert(!rendered.includes('<img'));
  assert(rendered.includes('class="chatOther">  [fake] [me] quoted'));
  for (const symbol of ['+','-','=']) {
    context.batterySymbol=symbol;
    assert(vm.runInContext('statValue("Battery",batterySymbol)',context).includes('<span class="batterySymbol">'));
    assert(vm.runInContext('statValue("Power","Battery: "+batterySymbol)',context).includes('<span class="batterySymbol">'));
  }
  assert(vm.runInContext('statValue("Power","<img>")',context).includes('&lt;img'));
  assert(!html.includes('Start logging') && !html.includes('Stop logging'));
  assert(!html.includes('Mark charging') && !html.includes('Mark resting'));
}

async function testSavedMessageTone(){
  const requests=[];
  context.fetch=async (url,options)=>{
    requests.push({url,options});
    return {ok:true,status:202,json:async()=>({toneMessage:'Gentle Alert',toneVolume:40})};
  };
  await vm.runInContext('testTone()',context);
  assert.equal(requests.length,1);
  assert.equal(requests[0].url,'/tone/test');
  assert.equal(requests[0].options.method,'POST');
  const args=new URLSearchParams(requests[0].options.body);
  assert(!args.has('style'),'Preview must use the saved message selection instead of forcing Chime');
  assert(!args.has('volume'),'Preview must preserve the saved volume');
  assert.equal(elements.toneStatus.textContent,'Queued message tone: Gentle Alert at 40%');
  context.fetch=async()=>({ok:false,status:409});
  await vm.runInContext('testTone()',context);
  assert(elements.toneStatus.textContent.includes('busy, muted, or recovering'));
  context.fetch=async()=>{throw new Error('offline');};
  await vm.runInContext('testTone()',context);
  assert.equal(elements.toneStatus.textContent,'Device unavailable');
}

function testOfflineCoverage(){
  vm.runInContext('renderOfflineCoverage({mapRequestedZoom:14,mapDisplayedZoom:12,mapCoverageChecked:true,mapCoverageAgeSec:2,mapCoverageLevels:[{zoom:12,status:"Ready",available:4,required:4,invalid:0},{zoom:14,status:"Partial",available:1,required:4,invalid:0}]})',context);
  assert(elements.mapCoverageStats.innerHTML.includes('Displayed zoom'));
  assert(elements.mapCoverageStats.innerHTML.includes('Ready (4/4)'));
  assert(elements.mapCoverageStats.innerHTML.includes('Partial (1/4)'));
  assert(elements.mapCoverageStatus.textContent.includes('touchscreen map center'));
  vm.runInContext('renderOfflineCoverage({mapCoverageChecked:true,mapCoverageLevels:[{zoom:14,status:"<img src=x>",available:0,required:1,invalid:1}]})',context);
  assert(!elements.mapCoverageStats.innerHTML.includes('<img'));
  assert(elements.mapCoverageStats.innerHTML.includes('&lt;img'));
  vm.runInContext('renderOfflineCoverage({mapCoverageChecked:false,mapDisplayedZoom:null})',context);
  assert(elements.mapCoverageStatus.textContent.includes('Open Map on the touchscreen'));
  assert(!elements.mapCoverageStats.innerHTML.includes('Ready'));
}

function testImperialMapScale(){
  let options;
  const fakeMap = {setView(){}};
  fakeMap.on=()=>{};
  const layer = {on(){},getContainer(){return {querySelectorAll(){return [];}};}};
  context.L = context.window.L = {
    map(){return fakeMap;},
    tileLayer(){return {addTo(){}};},
    control:{scale(value){options=value;return {addTo(map){assert.equal(map,fakeMap);}};}},
    GridLayer:{extend(){return function(){return layer;};}},
  };
  vm.runInContext('initRealMap()',context);
  assert.deepEqual(JSON.parse(JSON.stringify(options)),{imperial:true,metric:false});
}

function testBundledMapAssets(){
  const base=path.join(__dirname,'../src/vendor/leaflet');
  const manifest=JSON.parse(fs.readFileSync(path.join(base,'manifest.json'),'utf8'));
  const header=fs.readFileSync(path.join(__dirname,'../src/WebMapAssets.h'),'utf8');
  assert.equal(manifest.version,'1.9.4');
  for(const [name,hash] of Object.entries(manifest.sha256)){
    const raw=fs.readFileSync(path.join(base,name));
    assert.equal(crypto.createHash('sha256').update(raw).digest('hex'),hash,`${name} matches pinned hash`);
    if(name==='LICENSE')continue;
    const symbol=name.replace('.','_')+'_gz';
    const match=header.match(new RegExp(symbol+'\\[\\] PROGMEM = \\{([\\s\\S]*?)\\};'));
    assert(match,`${name} is bundled`);
    const zipped=Buffer.from([...match[1].matchAll(/0x([a-f0-9]{2})/g)].map(m=>parseInt(m[1],16)));
    assert.deepEqual(zlib.gunzipSync(zipped),raw,`${name} flash bundle matches local source`);
  }
  assert(!/<(?:script|link)[^>]+(?:src|href)="https?:/.test(html),'Map assets must load locally');
  assert(!html.includes('tile.openstreetmap.org'),'No online tile dependency');
}

async function testGameRomsAndConversationIsolation() {
  let requests = [], finishUpload;
  context.FormData = class { append(...values) { this.values = values; } };
  elements.romUploadFile.files = [{name:'../bad.gb',size:32768}];
  context.fetch = async (url, options) => { requests.push(url); return {ok:true,text:async()=> 'Uploaded',json:async()=>({roms:[]})}; };
  await vm.runInContext('romUploadForm.onsubmit({preventDefault(){}})',context);
  assert.equal(requests.length,0,'Traversal filename is blocked before upload');
  elements.romUploadFile.files = [{name:'Good.gb',size:4194305}];
  await vm.runInContext('romUploadForm.onsubmit({preventDefault(){}})',context);
  assert.equal(requests.length,0,'Oversized ROM is blocked');
  elements.romUploadFile.files = [{name:'Good.gb',size:32768}];
  context.fetch = async (url,options) => {
    requests.push(url);
    if(url==='/games/roms/upload')return new Promise(resolve=>{finishUpload=()=>resolve({ok:true,text:async()=> 'ROM uploaded'});});
    return {ok:true,json:async()=>({roms:[{name:'<img src=x onerror=alert(1)>.gb',bytes:32768}]})};
  };
  const upload=vm.runInContext('romUploadForm.onsubmit({preventDefault(){}})',context);
  await vm.runInContext('romUploadForm.onsubmit({preventDefault(){}})',context);
  assert.equal(requests.filter(x=>x==='/games/roms/upload').length,1,'Repeated upload click sends one request');
  finishUpload(); await upload;
  assert.equal(elements.romUploadButton.disabled,false);
  assert(!elements.romFiles.innerHTML.includes('<img'),'ROM names are escaped');
  let oldReply;
  requests=[];
  context.fetch=async (url,options)=>{
    requests.push([url,options]);
    if(url.startsWith('/messages/thread?')&&url.includes('00000002'))return new Promise(resolve=>{oldReply=()=>resolve({ok:true,json:async()=>({text:'OLD DEVICE SECRET'})});});
    return {ok:true,json:async()=>({text:'NEW DEVICE THREAD'}),text:async()=>''};
  };
  const first=vm.runInContext("openWebDirectConversation('!00000002')",context);
  elements.directMsg.value='Draft for device two';
  await vm.runInContext("openWebDirectConversation('!00000003')",context);
  oldReply(); await first;
  assert.equal(elements.directTo.value,'!00000003','Late data cannot change recipient');
  assert(!elements.directChatLog.innerHTML.includes('OLD DEVICE SECRET'),'Late thread response cannot cross conversations');
  assert(elements.directChatLog.innerHTML.includes('NEW DEVICE THREAD'));
  assert.equal(elements.directMsg.value,'','Draft from another device is not reused');
  assert.equal(requests.filter(([u,o])=>u==='/messages/read'&&o.body.get('peer')==='!00000002').length,0,'Hidden stale thread is not marked read');
  context.fetch=async()=>({ok:true,json:async()=>({text:'THREAD TWO'}),text:async()=>''});
  await vm.runInContext("openWebDirectConversation('!00000002')",context);
  assert.equal(elements.directMsg.value,'Draft for device two','Draft is restored only for its device');
  vm.runInContext("selectedWebDirectPeer='';currentPage='overview'",context);
}

(async () => {
  await testRendering();
  await testSingleRequestAndRetries();
  await testRequestTimeout();
  await testRetryRequests();
  await testSavedMessageTone();
  await testSerialPinSettings();
  await testBatteryCycleControls();
  testChatColorsAndBatterySymbols();
  testOfflineCoverage();
  testImperialMapScale();
  testBundledMapAssets();
  await testGameRomsAndConversationIsolation();
  console.log('Web UI regressions passed: escaping, single request, retry/backoff, recovery, timeout, delivery, runtime, retry controls, sound preview, serial pin readback/edit/save, coverage, imperial scale, bundled map assets.');
})().catch(error => { console.error(error); process.exitCode = 1; });
