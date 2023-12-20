# Minimalist Bar

## Building

### Install dependencies

#### Arch

```bash
pacman -S cjson i3-wm xorg-server
```

#### Ubuntu

```bash
sudo apt-get update
```

```bash
sudo apt-get install -y libpulse-dev libx11-dev libxft-dev libcjson-dev libbluetooth-dev libfreetype6-dev i3-wm libxrandr-dev xserver-xorg-dev
```

### Build the project

Building release build:

```bash
./build.sh
```

Building debug build:

```bash
./build.sh debug
```

Cleaning:

```bash
./build.sh clean
```

## TODO (sorted by priority)

- [ ] put all magic values in config file (see TODO in code)
- [ ] add a xwindow module (to display windows name on the left)
- [ ] allow empty config file, so use default values
- [ ] allow missing config file options
- [ ] read config file path from program arguments
- [ ] create wiki to explain at least the config file

## Stuff to add

- [ ] support including config file in config.json, so we can define a desktop.json config and a laptop.json, and include a base.json config in these files so we don't have to redefine common config options in each file
- [ ] reload the bar when the user modify and save the config file (use notify watch on the file)
- [ ] support custom modules
- [ ] add tray support
- [ ] support hover and clicks?
- [ ] handle text overflow of modules (carousel system?)
- [ ] allow custom icons for modules
