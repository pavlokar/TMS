const p = msg.payload;
const temp = Number(p.temperature);
const status = String(p.status || "unknown");
if (!Number.isFinite(temp)) return null;

msg.payload = `${temp.toFixed(1)} °C (${status})`;
return msg;