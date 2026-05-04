# ESP32

man muss in es32 quellcode hier const char* mqtt_host = "10.204.119.42"; die IP-Adresse anpassen, indem man Ip-Adresse von dem Gerät nimmt, worauf ein MQTT-Broker läuft.


# Generate Login-String for MQTT

```bash
docker run --rm -v $(pwd)/mosquitto/config:/mosquitto/config \
  eclipse-mosquitto mosquitto_passwd -b -c /mosquitto/config/passwd mqttuser mqttpass
```
## Start

```bash
docker compose -p cps-raspi up -d --build
```

## Recreate

```bash
docker compose -p cps-raspi up -d --no-deps --force-recreate
```

```bash
docker compose -p cps-raspi up -d --no-deps --force-recreate mosquitto
```
