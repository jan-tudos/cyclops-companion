# Companion for the Cyclops TUI
The OBSBOT webcam comes with a few (more or less useful) “AI” features, e.g. the ability to track the speaker.
Unfortunately, when this tracking is enabled it overrides manual adjustment commands.
Even worse, these features can be enable via gestures … which often happens unintentional.

There is a Windows/Macos software that can control these camera features, but

1) the software is horribly bloated (>100MB!?), and
2) there is no version available for GNU/Linux, let alone a FOSS one.

This tool provides a very limited set of most useful controls by directly sending the corresponding “magic USB packets” to the camera.
I obtained those packets by capturing the official software's communication; see `USB captures` for details.

Also see [Cyclops](https://github.com/jan-tudos/cyclops).

# Prerequisits
- webcam ;-)
- libusb (+ develment files for building)
- most useful in combination with Cyclops

# Setup
- clone the repo
- optional: adjust the camera device info (line 30–32)
- install libusb development files (`apt install libusb-1.0-0-dev` in Debian)
- `make`

# Usage
Make sure you have *write* permissions on the camera's device file
   - easiest option: run as root :o)
   - change permissions on the device file (The tool will report which file that is, in case permissions are missing.)
   - potentially automate one of the two previous options

The companion is one single binary with three different hardlinks.
The file's name determines its functionality when run:
- `siren`
   - wake camera from suspend
   - unlock from any tracked target
   - disable gesture for locking on a target
   - disable gesture for zooming
- `panacea`
   - wake camera from suspend
   - enable gesture for locking on a target
   - enable gesture for zooming
- `morpheus`
   - suspend camera
