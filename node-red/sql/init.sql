CREATE TABLE IF NOT EXISTS temperature_readings (
    id          BIGINT AUTO_INCREMENT PRIMARY KEY,
    device_id   VARCHAR(64)   NOT NULL,
    temperature DECIMAL(5,2)  NOT NULL,
    status      VARCHAR(32)   NOT NULL,
    ts          DATETIME      NOT NULL,
    INDEX idx_device_ts (device_id, ts)
);