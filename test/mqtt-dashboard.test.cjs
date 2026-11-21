/** @file mqtt-dashboard.test.cjs @brief Kiểm tra topic và ACK của browser MQTT client. */
const assert = require('node:assert/strict');
const { EventEmitter } = require('node:events');
const fs = require('node:fs');
const path = require('node:path');
const test = require('node:test');
const vm = require('node:vm');

class FakeClient extends EventEmitter {
  constructor() {
    super();
    this.connected = false;
    this.subscribed = [];
    this.sent = [];
  }

  subscribe(topics, options, callback) {
    this.subscribed = topics;
    callback(null);
  }

  publish(topic, payload, options, callback) {
    this.sent.push({ topic, payload, options });
    callback(null);
  }

  end() {
    this.connected = false;
  }
}

function createDashboard(protocol = 'http:') {
  const client = new FakeClient();
  const browser = {
    location: { protocol },
    mqtt: { connect: () => client },
  };
  const source = fs.readFileSync(path.join(__dirname, '..', 'mqtt-dashboard.js'), 'utf8');
  vm.runInNewContext(source, { window: browser, setTimeout, clearTimeout });
  return { client, DashboardMqtt: browser.DashboardMqtt };
}

test('subscribes to the device topics and correlates a config ACK', async () => {
  const { client, DashboardMqtt } = createDashboard();
  const received = [];
  const dashboard = new DashboardMqtt({ onTelemetry: (value) => received.push(value) });
  dashboard.connect({ url: 'ws://broker:9001', deviceId: 'esp32-01' });
  client.connected = true;
  client.emit('connect');

  assert.equal(client.subscribed.length, 5);
  assert.ok(client.subscribed.includes('watering/esp32-01/telemetry'));
  assert.ok(client.subscribed.includes('watering/esp32-01/config/ack'));

  const pending = dashboard.publishAction('set', { mf: {} });
  const sent = client.sent[0];
  const command = JSON.parse(sent.payload);
  assert.equal(sent.topic, 'watering/esp32-01/config/set');
  assert.equal(sent.options.qos, 1);
  assert.equal(sent.options.retain, false);
  assert.equal(command.action, 'set');
  client.emit('message', 'watering/esp32-01/config/ack',
    Buffer.from(JSON.stringify({ v: 1, requestId: command.requestId, ok: true, reason: 'ok' })));
  assert.equal((await pending).ok, true);

  client.emit('message', 'watering/esp32-01/telemetry',
    Buffer.from(JSON.stringify({ v: 1, valid: true, soilMoisturePct: 35 })));
  assert.equal(received[0].soilMoisturePct, 35);
  dashboard.disconnect();
});

test('rejects insecure WebSocket URLs from an HTTPS page', () => {
  const { DashboardMqtt } = createDashboard('https:');
  const dashboard = new DashboardMqtt();
  assert.throws(() => dashboard.connect({ url: 'ws://broker:9001', deviceId: 'esp32-01' }),
    /wss:\/\//);
});
