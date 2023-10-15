# Minimalist Bar

## TODO (sorted by priority)

- [ ] resynchronize clock when the process is suspsend (e.g. when the pc goes to sleep)
- [ ] put all magic values in config file (see TODO in code)
- [ ] read default interface from config file
- [ ] read default battery from config file
- [ ] add a xwindow module (to display windows name on the left)
- [ ] allow empty config file, so use default values

## Stuff to add

- [ ] support including config file in config.json, so we can define a desktop.json config and a laptop.json, and include a base.json config in these files so we don't have to redefine common config options in each file
- [ ] reload the bar when the user modify and save the config file (use notify watch on the file)
- [ ] support custom modules
- [ ] add tray support
- [ ] support hover and clicks?
- [ ] handle text overflow of modules (carousel system?)
