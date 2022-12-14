# Testing out the MQTT Plugins

This test walks through the steps for setting up both MQTT plugins and running them locally.

## Setup

- Install the [MQTT Status Plugin](https://github.com/robotastic/trunk-recorder-mqtt-status) & [MQTT Statistics Plugin](https://github.com/robotastic/trunk-recorder-mqtt-statistics)

- Starting Mosquitto on a Mac:
```bash
/opt/homebrew/sbin/mosquitto -c /opt/homebrew/etc/mosquitto/mosquitto.conf -v
```

- Connect an RTL-SDR to the testing computer.

## Testing

Run:
`trunk-recorder`