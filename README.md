# SFG14-73 Fan Control Utility

Set fan speed strategy for ``Acer Swift Go 14 (SFG14-73)`` on Linux.

> [!WARNING]  
> Please ensure that your device model is ``SFG14-73``. Writing the wrong EC address to a different machine could cause hardware damage!

## How to determine this program is sutiable for my device
This program will read and write EC register to achieve get and set fan mode. \
So the only thing we need to ensure is that the device uses the same EC register to store the fan mode. \
To check this, you can follow this [wiki](https://github.com/hirschmann/nbfc/wiki/Probe-the-EC's-registers) 

In my case (SFG14-73) the values are:
```c
#define EC_CMD_PORT  0x66
#define EC_DATA_PORT 0x62
#define EC_OFFSET 0x45
```
and fanmode:
```c
#define BALANCE_MODE 1
#define SLIENT_MODE 2
#define PERFORMANCE_MODE 3
```

## Installation 
1. Clone this repository
2. Compile the ``main.c`` file using ``gcc -o fanctl main.c`` to generate executable: ``fanctl``
3. Move ``fanctl`` to your desired location or consider add it to path

## Usage
Use command ``fanctl -h`` to get help

### Read Fanmode
```bash
sudo fanctl -r
# Output: EC RAM value=3
```

### Set Fanmode
```bash
sudo ./fanctl -w 3
# Output: Successfully set EC RAM 0x45 to 0x03
```

### Using with waybar
By passing the ``-f waybar`` parameter, the program will output in waybar json format
```bash
sudo ./fanctl -f waybar -r
# Output: {"value": 3, "text": "󰓅", "tooltip": "Fan Mode: Performance"}
```

## Waybar Setup
To add this as a widget in waybar, please follow the step below

1. Create an new entry in ``config.jsonc`` 
```json
"custom/fanmode": {
  "exec": "sudo fanctl -r -f waybar",
  "return-type": "json",
  "format": "{}",
  "on-click": "sudo fanctl -t && kill -s 44 $(pidof waybar)",
  "min-length": 1,
  "max-length": 1,
  "signal": 10,  // Refresh when SIGRTMIN+10 is revieced
}
```

2. Open passwordless sudo list using ``sudo visudo``, and add the code below  
```bash
<USERNAME> ALL=(ALL) NOPASSWD: /full/path/to/fanctl *
```