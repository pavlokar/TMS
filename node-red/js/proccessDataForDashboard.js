const p = msg.payload;
const temp = Number(p.temperature);
if (!Number.isFinite(temp)) return null;

msg.topic = p.device_id || "esp32-temp-01"; // Serienname
msg.payload = temp;                         // Wert für Chart
return msg;