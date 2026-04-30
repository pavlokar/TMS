const f = msg.payload || {};

let sql = `
SELECT ts, temperature, status, device_id
FROM temperature_readings
WHERE 1 = 1
`;

const params = [];

if (f.device_id && String(f.device_id).trim() !== "") {
    sql += " AND device_id = ?";
    params.push(String(f.device_id).trim());
}

if (f.from && String(f.from).trim() !== "") {
    sql += " AND ts >= ?";
    params.push(String(f.from).trim());
}

if (f.to && String(f.to).trim() !== "") {
    sql += " AND ts <= ?";
    params.push(String(f.to).trim());
}

if (f.min_temperature !== undefined && f.min_temperature !== null && f.min_temperature !== "") {
    const minTemp = Number(f.min_temperature);
    if (Number.isFinite(minTemp)) {
        sql += " AND temperature >= ?";
        params.push(minTemp);
    }
}

if (f.max_temperature !== undefined && f.max_temperature !== null && f.max_temperature !== "") {
    const maxTemp = Number(f.max_temperature);
    if (Number.isFinite(maxTemp)) {
        sql += " AND temperature <= ?";
        params.push(maxTemp);
    }
}

sql += " ORDER BY ts DESC";

let limit = Number(f.limit || 100);

if (!Number.isInteger(limit) || limit <= 0) {
    limit = 100;
}

if (limit > 1000) {
    limit = 1000;
}

sql += " LIMIT ?";
params.push(limit);

msg.topic = sql;
msg.payload = params;

return msg;