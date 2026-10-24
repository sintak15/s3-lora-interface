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
    fuelGaugePresent: true, batteryRuntimeMinutes: 125,
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
  assert.equal(elements.appTitle.textContent, attack);
  assert.equal(elements.radioHealth.textContent, 'Heltec: stale | last valid response: 95s ago');
  assert(elements.txHistoryBody.innerHTML.includes('retryMessage(123)'));
  assert(elements.txHistoryBody.innerHTML.includes('deliveryBadge failed'));
  assert.equal(elements.statusFreshness.textContent, 'Live · updated just now');
  assert.equal((elements.txHistoryBody.innerHTML.match(/retryMessage/g)||[]).length, 1);
  assert.equal(vm.runInContext('batteryRuntimeLabel({fuelGaugePresent:true,batteryRuntimeMinutes:125})', context), '~2h 5m');
  assert.equal(vm.runInContext('batteryRuntimeLabel({fuelGaugePresent:true,batteryRuntimeMinutes:-1})', context), 'Estimating remaining time');
  assert.equal(vm.runInContext('batteryRuntimeLabel({fuelGaugePresent:true,batteryRuntimeMinutes:128,batteryRuntimeBasis:"previous"})', context), '~2h 10m · previous use');
  assert.equal(vm.runInContext('batteryRuntimeLabel({fuelGaugePresent:true,batteryRuntimeMinutes:59,batteryRuntimeBasis:"gauge"})', context), '~1h 0m · gauge trend');
  assert.equal(vm.runInContext('batteryRuntimeLabel({fuelGaugePresent:true,batteryRuntimeMinutes:2,batteryRuntimeBasis:"current"})', context), '~0h 5m · current use');
  assert.equal(vm.runInContext('batteryRuntimeLabel({fuelGaugePresent:true,batteryRuntimeMinutes:0,batteryRuntimeBasis:"current"})', context), '~0h 0m · current use');
  assert.equal(vm.runInContext('batteryRuntimeLabel({fuelGaugePresent:true,batteryRuntimeMinutes:NaN})', context), 'Estimating remaining time');
  assert.equal(vm.runInContext('batteryRuntimeLabel({fuelGaugePresent:true,batteryRuntimeMinutes:Infinity})', context), 'Estimating remaining time');
  assert.equal(vm.runInContext('batteryRuntimeLabel({fuelGaugePresent:true,batteryRuntimeMinutes:125,batteryRuntimeBasis:"<img>"})', context), '~2h 5m');
  assert.equal(vm.runInContext('batteryRuntimeLabel({fuelGaugePresent:true,batteryCharging:true,batteryRuntimeMinutes:125})', context), 'Battery: Increasing');
  assert.equal(vm.runInContext('batteryRuntimeLabel({fuelGaugePresent:true,batteryChargeRateValid:false})', context), 'Battery: Unavailable');
  assert.equal(vm.runInContext('batteryRuntimeLabel({fuelGaugePresent:true,batteryGaugeTrend:"rising",batteryCharging:false})', context), 'Battery: Increasing');
  assert.equal(vm.runInContext('batteryRuntimeLabel({fuelGaugePresent:false})', context), 'Unavailable (no gauge)');
  for (const [direction,label] of [['rising','Increasing'],['falling','Decreasing'],['steady','Steady'],['unavailable','Unavailable']]) {
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

async function testBatteryCycleControls(){
  let calls=0,finish;
  context.fetch=(url,options)=>{
    calls++;
    assert.equal(url,'/battery/cycle');
    assert.equal(options.method,'POST');
    assert.equal(new URLSearchParams(options.body).get('action'),'charging');
    return new Promise(resolve=>{finish=resolve;});
  };
  const first=vm.runInContext("batteryCycleAction('charging')",context);
  await vm.runInContext("batteryCycleAction('charging')",context);
  assert.equal(calls,1,'Duplicate taps must not emit multiple markers');
  finish({ok:true,text:async()=> 'Recording to SD'});
  await first;
  assert.equal(elements.batteryCycleStatus.textContent,'Recording to SD');
  context.fetch=async()=>({ok:false,text:async()=> 'SD unavailable'});
  await vm.runInContext("batteryCycleAction('start')",context);
  assert.equal(elements.batteryCycleStatus.textContent,'Action failed: SD unavailable');
  context.fetch=async()=>{throw new Error('offline');};
  await vm.runInContext("batteryCycleAction('stop')",context);
  assert(elements.batteryCycleStatus.textContent.includes('result unknown'));
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

(async () => {
  await testRendering();
  await testSingleRequestAndRetries();
  await testRequestTimeout();
  await testRetryRequests();
  await testSavedMessageTone();
  await testBatteryCycleControls();
  testOfflineCoverage();
  testImperialMapScale();
  testBundledMapAssets();
  console.log('Web UI regressions passed: escaping, single request, retry/backoff, recovery, timeout, delivery, runtime, retry controls, sound preview, coverage, imperial scale, bundled map assets.');
})().catch(error => { console.error(error); process.exitCode = 1; });
