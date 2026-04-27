SELECT ts, temperature, status, device_id
FROM temperature_readings
WHERE device_id    = ?
  AND ts          >= ?
  AND ts          <= ?
  AND temperature >= ?
  AND temperature <= ?
ORDER BY ts DESC
LIMIT ?;
