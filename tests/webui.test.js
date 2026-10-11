const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const path = require('node:path');

const html = fs.readFileSync(path.join(__dirname, '../src/WebUi.h'), 'utf8');
const script = html.split('<script>')[1].split('</script>')[0];
// Compile the entire UI script, including handlers that are not exercised below.
new vm.Script(script);

function element() {
  return {
    value: '', textContent: '', innerHTML: '', dataset: {}, options: [],
    classList: { toggle() {}, contains() { return false; } },
    add(option) { this.options.push(option); }, addEventListener() {},
    replaceChildren() {},
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
  };
  context.fetch = async () => ({ ok: true, json: async () => status });
  await vm.runInContext('refresh()', context);
  assert(elements.configRef.innerHTML.includes('&lt;img'));
  assert(elements.channelRef.innerHTML.includes('&lt;img'));
  for (const id of ['stats', 'firmwareStats', 'storageStats', 'configRef', 'diagStats', 'packetStats', 'channelRef']) {
    assert(!elements[id].innerHTML.includes('<img'), `${id} must escape dynamic HTML`);
  }
  assert.equal(elements.appTitle.textContent, attack);
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

(async () => {
  await testRendering();
  await testSingleRequestAndRetries();
  await testRequestTimeout();
  console.log('Web UI regressions passed: escaping, single request, retry/backoff, recovery, timeout.');
})().catch(error => { console.error(error); process.exitCode = 1; });
