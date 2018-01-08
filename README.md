# fastboot3DS
_A bootloader for the 3DS console, written by derrek, profi200, d0k3_

fastboot3DS is a bootloader for the 3DS console, intended to be installed to the FIRM0 partition. It allows you to assign homebrew in .firm format to bootslots, and to chainload bootslots via either a bootmenu or a button combo of your choosing. fastboot3DS also contains basic tools for NAND backups and restores.

![fastboot3DS on a real N3DS](https://github.com/derrekr/fastboot3DS/blob/master/assets/fastboot_on_n3ds.jpg?raw=true)

## Disclaimer
fastboot3DS installs to a critical partition of your system, and thus becomes one of the softwares critical to the functioning of your system. Although having been thoroughly tested, some risk may still remain, especially when fastboot3DS is not used in the way it is intended. In short: we are not responsible for any damage that may occur to your system as a direct or indirect result of you using fastboot3DS.

## Quick start guide
These short instructions require you to have a way of booting [OpenFirmInstaller](https://github.com/d0k3/OpenFirmInstaller). If you already have [boot9strap](https://github.com/SciresM/boot9strap) installed, this is as simple as chainloading the `OpenFirmInstaller.firm` (either via some chainloader of your choosing, or put it on your SD card as `sdmc:/boot.firm`).
* Have `fastboot3DS.firm` inside the `sdmc:/ofi`folder on your SD card. When installing from A9LH, `secret_sector.bin` is also required.
* Boot OpenFirmInstaller and follow the on screen instructions. You will reboot to the fastboot3DS menu.
* [optional] Enter `Boot setup...` -> `Setup [slot 1]...` -> `Select [slot 1] firm` and select the FIRM file of your main CFW. On typical systems that is `smdc:/boot.firm`, but anything goes.
* [optional] Enter `Boot setup...` -> `Change boot mode...` -> `Set quiet boot`. Your system is now set to autoboot and will silently boot the CFW you selected above.

You may also want to set up the other boot slots and assign key combos to them. Keep in mind you need one autoboot slot (= a slot with no key combo assigned). If you want to access the fastboot3DS menu at a later point in time, hold the HOME button when powering on the console. From the fastboot3DS menu, you may continue the boot process via `Continue boot`, chainload a .firm file via `Boot from file...`, access the boot menu via `Boot menu...` or power off the console via the POWER button.

## How to build
To compile fastboot3DS you need [devkitARM](https://sourceforge.net/projects/devkitpro/), [CTR firm builder](https://github.com/derrekr/ctr_firm_builder) and [splashtool](https://github.com/profi200/splashtool) installed in your system. Additionally you need 7-Zip or on Linux p7z installed to make release builds. Also make sure the CTR firm builder and splashtool binaries are in your $PATH environment variable and accessible to the Makefile. Build fastboot3DS as debug build via `make` or as release build via `make release`.

## Known issues
This section is reserved for a listing of known issues. At present only this remains:
* Older releases of [GodMode9](https://github.com/d0k3/GodMode9) freeze when they are chainloaded via fastboot3DS. Use v1.5.0 or higher. In general (that means not only for fastboot3ds) it is recommended to have all your software updated to the latest version.
* OpenFirmInstaller only allows installing official (= signed) releases of fastboot3ds. Developers wanting to test their own builds can install fastboot3ds custom builds via GodMode9.

If you happen to stumble over another bug, please open an issue in the [official fastboot3DS repo on GitHub](https://github.com/derrekr/fastboot3DS/issues) or contact us via other platforms.

## License
You may use this under the terms of the GNU General Public License GPL v3 or under the terms of any later revisions of the GPL. Refer to the provided `LICENSE.txt` file for further information.

## Thanks to...
* **yellows8**
* **plutoo**
* **smea**
* **Normmatt** (for sdmmc code)
* **WinterMute** (for console code)
* **ctrulib devs** (for HID code)
* **Luma 3DS devs** (for fmt.c/gfx code)
* **mtheall** (for LZ11 decompress code)
* **devkitPro** (for the toolchain/makefiles)
* **ChaN** (for the FATFS library)
* **fincs**, **Al3x_10m**, **Wolfvak**, **Shadowhand**, **Lilith Valentine**, **Crimson**, **Ordim3n** (closed beta testing)
* ...everyone who contributed to **3dbrew.org**

Copyright (C) 2017 derrek, profi200, d0k3
