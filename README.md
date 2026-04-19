# is-keyboard-plugged

Detects whether a USB keyboard is connected by inspecting `/sys/bus/usb/devices/`.

An interface is considered a keyboard when both conditions hold:
- `bInterfaceClass` = `03` (HID)
- `bInterfaceProtocol` = `01` (boot keyboard)

## Usage

```
is-keyboard-plugged [--verbose] [--help]
```

| Option | Description |
|--------|-------------|
| `--verbose` | Print matched device info, or `No USB keyboards.` if none found |
| `--help` | Show usage and exit |

**Exit codes**

| Code | Meaning |
|------|---------|
| 0 | At least one USB keyboard is connected |
| 1 | No USB keyboard found |

### Example

```sh
# Silent check (suitable for shell conditionals)
if is-keyboard-plugged; then
    echo "Keyboard present"
fi

# Human-readable output
is-keyboard-plugged --verbose
# Acme Corp SuperKeyboard [04d9:0024] (interface: /sys/bus/usb/devices/1-2:1.0)
```

## Build & Install

```sh
make
sudo make install                        # installs to /usr/local/bin
sudo make install PREFIX=/usr            # installs to /usr/bin
make install DESTDIR=/tmp/staging        # for package managers / ebuild
```

## systemd integration example

Intended use case: a digital signage system that boots directly into a media
player (KMS/DRM, no display server required). When a USB keyboard is detected
at boot, the player is skipped and the system falls back to the normal login
prompt provided by `getty@tty1.service`.

```ini
# /etc/systemd/system/maintenance-notice.service
[Unit]
Description=Write maintenance mode notice to login screen
Before=getty@tty1.service
After=local-fs.target

[Service]
Type=oneshot
# Runs only when a keyboard is connected; writes a notice to /run/issue.d/
# so that agetty displays it before the login prompt.
ExecCondition=/usr/local/bin/is-keyboard-plugged
ExecStart=/bin/sh -c 'mkdir -p /run/issue.d && printf "\nUSB keyboard detected: entering maintenance mode.\n\n" > /run/issue.d/50-maintenance.issue'

[Install]
WantedBy=multi-user.target
```

```ini
# /etc/systemd/system/signage.service
[Unit]
Description=Digital Signage
After=network.target

[Service]
# Skip this service when a USB keyboard is connected.
ExecCondition=/usr/local/bin/is-keyboard-plugged --inverse
ExecStart=/usr/bin/mpv rtmp://your-rtmp-server/live/your-broadcast
Restart=on-failure
RestartSec=3

[Install]
WantedBy=multi-user.target
```

Enable both units:

```sh
systemctl enable maintenance-notice.service signage.service
```

With `multi-user.target`, `getty@tty1.service` runs unconditionally.
When a keyboard is connected, `signage.service` is skipped and
`maintenance-notice.service` writes a notice to `/run/issue.d/` (read by
agetty automatically), so the login prompt is preceded by an explanatory
message. The file lives under `/run/` and is cleared on the next reboot.

## Requirements

- Linux with sysfs (`/sys/bus/usb/devices/`)
- C99 compiler
