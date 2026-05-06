# keysound 

A high-performance, low-latency mechanical keyboard sound daemon for Linux. It works by hooking directly into the kernel input subsystem, making it fully compatible with **Wayland** (Hyprland, Sway, GNOME, etc.) and **X11**.

## Features
- **Low Latency:** Reads events directly from `/dev/input/` via `libevdev`.
- **Display Agnostic:** Works on Wayland and X11.
- **Hot-swapping:** Automatically detects when keyboards are plugged or unplugged using `udev`.
- **Low Resource Usage:** Written in pure C; uses `poll()` to stay idle when you aren't typing.
- **Soundpack Management:** Includes `keysound-ctl` to switch soundpacks even you daemon is running.

## Installation

> **Important:** After installing, you **must** complete the Post-Installation Setup to give the daemon permission to read your keyboard.  
> Without this step, **no sounds will play**.

### Arch Linux (AUR)
If you are on Arch, you can install it using your favorite AUR helper:
```bash
yay -S keysound
```

### Manual Build
Ensure you have the following dependencies:
- `libevdev`
- `libudev`
- `miniaudio`
- `clang`

   ```bash
   git clone https://github.com/Shyam-Jagath/keysound.git
   cd keysound
   make
   sudo make install
   ```

## Post-Installation Setup
Because `keysound` reads raw input events, your user needs permission to access input devices.

1. Add your user to the `input` group:
   ```bash
   sudo usermod -aG input $USER
   ```
2. **Log out and log back in** (or reboot) for the group changes to take effect.
3. Enable and start the background daemon:
   ```bash
   systemctl --user enable --now keysound.service
   ```

## Usage

### Changing Soundpacks
Use the included control tool to list and switch between soundpacks even if the dameon is running:
```bash
keysound-ctl
```

### Creating Your Own Soundpack
Soundpacks are stored in `~/.config/keysound/` or `/usr/share/keysound/soundpacks/`.
Each pack should have the following folder structure:
```text
my-cool-pack/
├── ALPHANUMERIC/
├── BACKSPACE_TAB/
├── ENTER/
├── LOCK/
├── MODIFIERS/
└── SPACE/
```
Simply drop your `.wav` or `.mp3` files into the corresponding folders. The daemon will randomly pick one from the folder each time the key is pressed.

## Contributing

Contributions are more than welcome!
If you have any ideas on how to make this project better, just raise an issue and discuss it there.
Your feedback, suggestions, and improvements are appreciated!
