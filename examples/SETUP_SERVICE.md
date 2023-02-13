# Automatically Starting Using a Service

## Install

Modify the example serivce: [trunk-recorder.service](trunk-recorder.service)

Install it by copying it to the **/etc/systemd/system** folder:

```bash
sudo cp trunk-recorder.service /etc/systemd/system
```

And now start it up!

```bash
sudo systemctl start trunk-recorder
```

## Autostarting

If you want to have the Service start automatically each time the system boots, you need to enable it:

```bash
sudo systemctl enable trunk-recorder.service
```


## Status

You can check the most recent output from the service:

```bash
sudo service trunk-recorder status
```

And you can follow the output if you want too:

```bash
journalctl -u trunk-recorder.service -f
```

## More info

Here is a [good article](https://www.digitalocean.com/community/tutorials/how-to-use-systemctl-to-manage-systemd-services-and-units) on using Services with Systemd.
