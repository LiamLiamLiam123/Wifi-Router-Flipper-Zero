# WifiRouter

<p align="center">
  <img src="assets/icon_menu_128x64px.png" alt="WifiRouter Icon" width="128"/>
</p>

Flipper Zero app that makes the ESP32 WiFi devboard broadcast **beacon frames as a SoftAP** (joinable Wi-Fi network). The network appears in **Settings → Wi-Fi** on the Flipper Zero.

---

## Features

- 📡 **Broadcast SoftAP** — ESP32 devboard becomes a visible Wi-Fi access point
- ⚙️ **Configurable SSID / Password / Channel** (1–13)
- 🔓 **Open or WPA2-PSK** — leave password empty for open network
- 🔄 **Toggle on/off** from the Flipper Zero UI
- 💾 **Settings persisted** across reboots
- 📋 **Clean UI** — Submenu → Router / Settings / About

---

## Requirements

| Component | Version / Notes |
|-----------|-----------------|
| **Flipper Zero** | Any firmware with GPIO app support |
| **ESP32 WiFi Devboard** | ESP32, ESP32-S2, or ESP32-C3 running **FlipperHTTP firmware** |
| **FlipperHTTP Firmware** | Must implement 3 custom UART commands (see below) |

### Required FlipperHTTP Firmware Commands

The ESP32 firmware **must** handle these UART strings sent by the app:

| Command | Example | Action |
|---------|---------|--------|
| Start AP | `[WIFI/AP/START]{"ssid":"MyAP","password":"secret","channel":6}` | `esp_wifi_set_mode(WIFI_MODE_AP)` → config → `esp_wifi_start()` |
| Stop AP | `[WIFI/AP/STOP]` | `esp_wifi_stop()` → `esp_wifi_set_mode(WIFI_MODE_NULL)` |
| Query Status | `[WIFI/AP/STATUS]` | Reply `[AP/ACTIVE]` or `[AP/IDLE]` |

> **If you build your own FlipperHTTP firmware**, add handlers in the command parser for these three strings and call the ESP-IDF WiFi AP APIs with the parsed JSON parameters.

---

## Installation

### 1. Flash FlipperHTTP to ESP32 Devboard
```bash
# Option A: Pre-built (from FlipperHTTP releases)
esptool.py --port /dev/ttyUSB0 write_flash 0x0 flipper_http_merged.bin

# Option B: Build from source
git clone https://github.com/jblanked/FlipperHTTP
cd FlipperHTTP
# Add the 3 command handlers, then:
idf.py build flash
```

### 2. Build & Install WifiRouter App
```bash
git clone https://github.com/LiamLiamliam123/Hello-World-Flipper-Zero
cd Hello-World-Flipper-Zero
ufbt                    # builds dist/wifirouter.fap
ufbt launch             # installs & runs on connected Flipper Zero
```

---

## Usage

1. Connect ESP32 devboard to Flipper Zero via GPIO (UART pins)
2. Open **Apps → GPIO → WifiRouter**
3. **Settings** → set **SSID**, **Password** (empty = open), **Channel** (1–13)
4. **Router** → press **OK** → "AP started - check WiFi"
5. On Flipper: **Settings → Wi-Fi** → your network appears
6. Press **OK** again in Router view to stop broadcasting

---

## Project Structure

```
├── app.cpp / app.hpp              # Main app, view dispatcher, storage, FlipperHTTP handle
├── router/router.cpp / router.hpp # Custom View: status screen + OK toggle
├── settings/settings.cpp / .hpp   # VariableItemList: SSID / Pass / Channel inputs
├── about/about.cpp / about.hpp    # About widget
├── flipper_http/                  # UART protocol (flipper_http.c/h + new AP commands)
├── easy_flipper/                  # UI helpers (Submenu, Widget, VariableItemList, TextInput)
├── assets/                        # App icon (128x64)
├── application.fam                # ufbt manifest (appid="wifirouter")
└── dist/wifirouter.fap            # Built package (after ufbt)
```

---

## Screenshots

| Submenu | Router View (Stopped) | Router View (Broadcasting) | Settings |
|---------|----------------------|---------------------------|----------|
| ![](docs/submenu.png) | ![](docs/router_stopped.png) | ![](docs/router_active.png) | ![](docs/settings.png) |

*Add screenshots to `docs/` folder*

---

## Building from Source

```bash
# Prerequisites: ufbt, arm-none-eabi toolchain (installed via ufbt)
git clone https://github.com/LiamLiamliam123/Hello-World-Flipper-Zero
cd Hello-World-Flipper-Zero
ufbt                    # debug build → dist/wifirouter.fap
ufbt release            # release build
```

Output: `dist/wifirouter.fap` — copy to Flipper Zero `/apps/GPIO/` or use `ufbt launch`.

---

## Firmware Development Notes

### Adding AP Support to FlipperHTTP Firmware

In `flipper_http` firmware command parser (e.g., `main.c` or command handler):

```c
// Pseudocode - adapt to actual parser
if (strstr(rx_line, "[WIFI/AP/START]")) {
    // Parse JSON: ssid, password, channel
    // wifi_config_t cfg = { .ap = { .ssid = ..., .password = ..., .channel = ..., .authmode = WIFI_AUTH_WPA2_PSK }};
    // esp_wifi_set_mode(WIFI_MODE_AP);
    // esp_wifi_set_config(WIFI_IF_AP, &cfg);
    // esp_wifi_start();
    // uart_send("[SUCCESS]");
}
else if (strstr(rx_line, "[WIFI/AP/STOP]")) {
    esp_wifi_stop();
    esp_wifi_set_mode(WIFI_MODE_NULL);
    uart_send("[SUCCESS]");
}
else if (strstr(rx_line, "[WIFI/AP/STATUS]")) {
    uart_send(ap_running ? "[AP/ACTIVE]" : "[AP/IDLE]");
}
```

---

## License

MIT — see [LICENSE](LICENSE)

---

## Credits

- **FlipperHTTP** by [@jblanked](https://github.com/jblanked) — UART bridge firmware & protocol
- **easy_flipper** helpers — UI boilerplate reduction
- **Flipper Zero** community documentation & examples

---

## Links

- [FlipperHTTP Firmware](https://github.com/jblanked/FlipperHTTP)
- [Flipper Zero Docs](https://docs.flipperzero.one/)
- [ESP-IDF WiFi AP Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_wifi.html#_CPPv417esp_wifi_set_mode14wifi_mode_t)