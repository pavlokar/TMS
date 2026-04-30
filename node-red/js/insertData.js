// msg.payload ist das JSON-Objekt vom ESP32
const p = msg.payload;

const deviceId = p.device_id || "esp32-temp-01";
const temp = Number(p.temperature);
const status = String(p.status || "unknown");

if (!Number.isFinite(temp)) return null;

// Node-RED kann "now" nehmen, oder du nimmst Zeit vom ESP
const ts = new Date().toISOString().slice(0, 19).replace('T', ' ');

msg.topic = "INSERT INTO temperature_readings (device_id, temperature, status, ts) VALUES (?, ?, ?, ?)";
msg.payload = [deviceId, temp, status, ts];
return msg;
