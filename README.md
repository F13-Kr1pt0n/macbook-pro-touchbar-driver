
---

# macbook12-spi-driver (fork, MBP14,3 compatible)

Input driver for the SPI keyboard / trackpad found on 12" MacBooks (2015 and later) and newer MacBook Pros (late 2016 through mid-2018), as well as a simple Touch Bar and ambient-light-sensor driver for late 2016 MacBook Pros and later.

The keyboard / trackpad driver here is now included in the kernel as of v5.3.

---

## About this fork

I own a **MacBookPro14,3 (15" 2017 with Touch Bar)** and created this fork to update the iBridge / Touch Bar drivers for **Linux 6.15+ compatibility**.

Recent kernels introduced official upstream drivers for the Touch Bar (keyboard vs display modes, DRM, HID splits). Unfortunately, this broke the original out-of-tree modules: the keyboard worked but the Touch Bar display/touch did not.

This fork:

* Adds a **Touch Bar mode coordinator** (`tb_mode=auto|keyboard|display`) so one driver owns both configs.
* Provides a minimal **multi-touch shim** for Touch Bar input on 6.15–6.16 (upstream HID covers this in 6.17+).
* Preserves all existing features (FN-mode toggling, idle/dim timeouts, ALS).
* Stays compatible with older kernels (<6.15) using legacy config logic.
* Avoids conflicts with upstream `hid-appletb-kbd`, `hid-appletb-bl`, and `appletbdrm`.

---

## NOTE

The touchbar driver was refactored in late 2018; if you're upgrading from the `appletb` driver, please see the [Upgrading](#upgrading) section.
If you're running a kernel before 4.16 then please check out the [legacy](../../tree/touchbar-driver-monolithic) branch instead.

---

## Using it

On MacBook / MacBook Pros (except MacBook8,1 2015):

* Kernels <4.11: boot with `intremap=nosid` (do **not** use `noapic`).
* MacBook8,1 (2015): recompile with `CONFIG_X86_INTEL_LPSS=n` if <4.14.
* Ensure SPI + LPSS modules are present (`spi_pxa2xx_platform`, `spi_pxa2xx_pci` or `intel_lpss_pci` depending on model).

**Initramfs tip**: put `applespi`, `spi_pxa2xx_platform`, and `intel_lpss_pci` in your initramfs so the keyboard works at disk password prompt.

---

## Touch Bar / ALS / iBridge

Three modules are provided:

* `apple_ibridge` — iBridge MFD coordinator.
* `apple_ib_tb` — Touch Bar driver.
* `apple_ib_als` — Ambient light sensor.

### Touch Bar features

* Basic mode switching (escape, fn keys, special keys).
* Idle/dim/off based on timeouts (`idle_timeout`, `dim_timeout` params).
* **New in this fork**:

  * `tb_mode_param`: `auto|keyboard|display` (default `keyboard`; `auto`/`display` pick configuration 2 on the T1).
  * Multi-touch reporting in display mode on 6.15–6.16.
  * `prefer_apple_ib` param to let this fork override upstream drivers.

### ALS

Exposes the ambient light sensor; works automatically with `iio-sensor-proxy`.

---

## Installation with DKMS

```bash
sudo pacman -S dkms linux-headers   # Arch
# or: sudo apt install dkms build-essential linux-headers-$(uname -r)

git clone https://github.com/F13-Kr1pt0n/macbook-pro-touchbar-driver.git
cd macbook12-spi-driver
sudo mkdir -p /usr/src/appleibridge-0.1
sudo cp -r . /usr/src/appleibridge-0.1

sudo dkms add -m appleibridge -v 0.1
sudo dkms build -m appleibridge -v 0.1
sudo dkms install -m appleibridge -v 0.1
```

Rebuild initramfs if not done automatically:

```bash
sudo mkinitcpio -P     # Arch
# or
sudo dracut -f         # Fedora/others
```

Load:

```bash
sudo modprobe apple-ibridge
sudo modprobe apple-ib-tb
sudo modprobe apple-ib-als
```

Enable the Touch Bar reset service:

```bash
sudo cp touchbar-reset.service /etc/systemd/system/
sudo systemctl enable --now touchbar-reset.service
```

### MacBookPro14,2 / T1 (05ac:8600) notes

Tested on a MacBookPro14,2 (13" 2017, four Thunderbolt ports) with Ubuntu 26.04, kernel 7.0.

* **`tb_mode_param` defaults to `keyboard`.** On the T1, `auto`/`display` select configuration 2
  (vendor-specific, CDC and audio interfaces besides HID); on the MacBookPro14,2 the Touch Bar
  works in configuration 1.
* **hid-sensor-hub can take the Touch Bar interface.** The interface with the Touch Bar
  display control also carries the light sensor. If `hid-sensor-hub` binds it first, the
  Touch Bar stays dark. Install the udev rule
  to hand it over (the light sensor keeps working through `apple-ib-als`):

  ```bash
  sudo cp udev/90-apple-ibridge-t1.rules /etc/udev/rules.d/
  sudo udevadm control --reload
  ```

  Check with `ls -l /sys/bus/hid/devices/*05AC:8600*/driver`: every entry should point to
  `apple-ibridge-hid`.
* **usbmuxd 1.1.1 switches the T1 off.** Its udev rule (`39-usbmuxd.rules`) sets
  `bConfigurationValue` to 0 for `05ac:8600`, which removes all T1 interfaces (Touch Bar,
  light sensor, FaceTime camera). Fixed upstream in usbmuxd commit `61b99ab`
  ("udev: Make sure iBridge (T1) doesn't end up in an unconfigured state", 2023), but
  distributions still shipping 1.1.1 lack it. Until then, copy the rule to
  `/etc/udev/rules.d/39-usbmuxd.rules` and drop `|5ac/8600/*` from the line that sets
  `bConfigurationValue="0"`.
* **Light sensor polling.** The T1 does not send illuminance reports by itself, so
  `apple-ib-als` polls it while it is in use: `poll_interval` (ms, default 5000, 0 = off).
  Each poll wakes the T1; at intervals below the USB autosuspend delay (2 s by default)
  it never powers down.

---

## What doesn’t work

* ISO layout autodetection.
* Resume quirks on MacBook8,1 (unchanged).

---

## Upgrading

Older `appletb` driver has been split into `apple_ibridge`, `apple_ib_tb`, `apple_ib_als`.
Remove any `appletb.ko` before installing this fork.

---

## Debugging

Trace Touch Bar events:

```bash
echo 1 | sudo tee /sys/kernel/debug/tracing/events/applespi/applespi_keyboard_data/enable
```

ALS / Touchpad logging available under `/sys/kernel/debug/applespi/`.

---

## Authors & Contributors

This project has been maintained and extended by several people over the years:

cb22 — started the original macbook12-spi-driver project to support Apple SPI keyboard and trackpad on early MacBooks.

roadrunner2 (Ronald Tschalär) — significantly expanded the driver, refactored Touch Bar/ALS support into proper iBridge subdrivers, and upstreamed the SPI keyboard driver (merged in Linux 5.3).

marc-git — maintained the repo afterwards, provided DKMS packaging, and kept the driver usable for later kernels.

F13-Kr1pt0n — forked the project as an MBP14,3 owner, updating the iBridge and Touch Bar stack for Linux 6.15+ compatibility (mode coordinator, HID MT shim, blacklist handling) while preserving backward compatibility with older kernels.

---

## License

This project is licensed under the **GPL-2.0** license.

This fork (MBP14,3 compatibility updates) was created and is maintained by **David Rodriguez**, specifically to restore Touch Bar and keyboard functionality on modern Linux kernels.

See the file headers (`// SPDX-License-Identifier: GPL-2.0`) for details.

---
