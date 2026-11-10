/**
 * @file mqtt-dashboard.js
 * @brief MQTT over WebSocket cho dashboard; không chứa code giao diện.
 *
 * Cần tải MQTT.js trước file này. Mỗi action có requestId và đợi ACK từ ESP32.
 * Broker phải mở WebSocket listener riêng cho trình duyệt.
 */
(function (global) {
  'use strict';

  class DashboardMqtt {
    /**
     * @param {Object} handlers Các callback onConnection/onStatus/onTelemetry/
     * onPump/onConfig/onAck/onError để giao diện cập nhật trạng thái.
     */
    constructor(handlers = {}) {
      this.handlers = handlers;
      this.client = null;
      this.baseTopic = '';
      this.pending = new Map();
    }

    /** @returns {boolean} Broker đang kết nối. */
    isConnected() {
      return Boolean(this.client && this.client.connected);
    }

    /**
     * @brief Kết nối broker bằng MQTT.js qua ws:// hoặc wss://.
     * @param {Object} settings {url, deviceId, username, password}.
     */
    connect(settings) {
      if (!global.mqtt || typeof global.mqtt.connect !== 'function') {
        throw new Error('Không tải được MQTT.js. Kiểm tra kết nối Internet/CDN.');
      }
      const url = settings.url.trim();
      const deviceId = settings.deviceId.trim();
      if (!/^wss?:\/\//i.test(url)) {
        throw new Error('Dashboard cần URL ws:// hoặc wss:// của broker.');
      }
      if (global.location.protocol === 'https:' && url.startsWith('ws://')) {
        throw new Error('Trang HTTPS cần kết nối broker bằng wss://.');
      }
      if (!/^[A-Za-z0-9_-]{1,40}$/.test(deviceId)) {
        throw new Error('Device ID chỉ gồm chữ, số, dấu _ hoặc -.');
      }

      this.disconnect();
      this.baseTopic = `watering/${deviceId}`;
      const options = {
        clientId: `dashboard-${Date.now().toString(36)}-${Math.random().toString(36).slice(2, 8)}`,
        clean: true,
        protocolVersion: 4,
        reconnectPeriod: 3000,
        connectTimeout: 5000,
      };
      if (settings.username) options.username = settings.username;
      if (settings.password) options.password = settings.password;

      this.client = global.mqtt.connect(url, options);
      this.client.on('connect', () => {
        const topics = ['status', 'telemetry', 'pump/state', 'config/state', 'config/ack']
          .map((suffix) => `${this.baseTopic}/${suffix}`);
        this.client.subscribe(topics, { qos: 1 }, (error) => {
          if (error) this._emit('onError', `Subscribe thất bại: ${error.message}`);
        });
        this._emit('onConnection', true);
      });
      this.client.on('message', (topic, payload) => this._handleMessage(topic, payload));
      this.client.on('close', () => this._emit('onConnection', false));
      this.client.on('error', (error) => this._emit('onError', error.message));
    }

    /** @brief Ngắt kết nối và từ chối các request đang chờ ACK. */
    disconnect() {
      if (this.client) {
        this.client.end(true);
        this.client = null;
      }
      for (const request of this.pending.values()) {
        clearTimeout(request.timer);
        request.reject(new Error('MQTT đã ngắt kết nối.'));
      }
      this.pending.clear();
      this._emit('onConnection', false);
    }

    /**
     * @brief Gửi action get/set/reset, đợi ACK ứng dụng tối đa 10 giây.
     * @returns {Promise<Object>} Bản tin ACK khi ESP32 đã xử lý.
     */
    publishAction(action, config) {
      if (!this.isConnected()) return Promise.reject(new Error('Chưa kết nối MQTT.'));
      if (!['get', 'set', 'reset'].includes(action)) {
        return Promise.reject(new Error('Action MQTT không hợp lệ.'));
      }
      const requestId = `${Date.now().toString(36)}-${Math.random().toString(36).slice(2, 8)}`;
      const message = { requestId, action };
      if (action === 'set') message.config = config;

      return new Promise((resolve, reject) => {
        const timer = setTimeout(() => {
          this.pending.delete(requestId);
          reject(new Error('ESP32 không ACK trong 10 giây.'));
        }, 10000);
        this.pending.set(requestId, { resolve, reject, timer });
        this.client.publish(`${this.baseTopic}/config/set`, JSON.stringify(message),
          { qos: 1, retain: false }, (error) => {
            if (!error) return;
            const request = this.pending.get(requestId);
            if (!request) return;
            clearTimeout(request.timer);
            this.pending.delete(requestId);
            reject(error);
          });
      });
    }

    _handleMessage(topic, payload) {
      const suffix = topic.slice(this.baseTopic.length + 1);
      if (!topic.startsWith(`${this.baseTopic}/`)) return;
      if (suffix === 'status') {
        this._emit('onStatus', payload.toString());
        return;
      }

      let data;
      try {
        data = JSON.parse(payload.toString());
      } catch (error) {
        this._emit('onError', `JSON lỗi tại ${suffix}: ${error.message}`);
        return;
      }
      if (!data || typeof data !== 'object' || Array.isArray(data)) {
        this._emit('onError', `Payload ${suffix} phải là JSON object.`);
        return;
      }
      if (data.v !== 1 && suffix !== 'config/state') return;

      if (suffix === 'telemetry') this._emit('onTelemetry', data);
      else if (suffix === 'pump/state') this._emit('onPump', data);
      else if (suffix === 'config/state') this._emit('onConfig', data);
      else if (suffix === 'config/ack') {
        this._emit('onAck', data);
        const request = this.pending.get(data.requestId);
        if (!request) return;
        clearTimeout(request.timer);
        this.pending.delete(data.requestId);
        if (data.ok) request.resolve(data);
        else request.reject(new Error(`ESP32 từ chối: ${data.reason || 'unknown'}`));
      }
    }

    _emit(name, value) {
      if (typeof this.handlers[name] === 'function') this.handlers[name](value);
    }
  }

  global.DashboardMqtt = DashboardMqtt;
})(window);
