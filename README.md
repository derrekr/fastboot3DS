# fastboot3DS
_A bootloader for the 3DS console, written by derrek, profi200, d0k3_

fastboot3DS is a bootloader for the 3DS console, intended to be installed to the FIRM0 partition. It allows you to assign homebrew in .firm format to bootslots, and to chainload bootslots via either a bootmenu or a button combo of your choosing. fastboot3DS also contains basic tools for NAND backup and restores.

## Disclaimer
fastboot3DS installs to a critical partition of your system, and thus becomes one of the softwares critical to the functioning of your system. Although having been thoroughly tested, some risk may still remain, especially when fastboot3DS is not used in the way it is intended. In short: we are not responsible for any damage that may occur to your system as a direct or indirect result of you using fastboot3DS.

## Quick start guide
These short instructions require you to have a way of booting [GodMode9](https://github.com/d0k3/GodMode9). If you already have [boot9strap](https://github.com/SciresM/boot9strap) installed, this is as simple as chainloading the `GodMode9.firm` (either via some chainloader of your choosing, or put it on your SD card as `sdmc:/boot.firm`).
* Have `fastboot3DS.firm` somewhere on your SD card.
* Start GodMode9, navigate to `fastboot3DS.firm`, A button -> `FIRM image options...` -> `Install FIRM` -> `Install to FIRM0` -> Enter the displayed button combo.
* Reboot your system via the START button. You will reboot to the fastboot3DS menu.
* [optional] Enter `Boot setup...` -> `Setup [slot 1]...` -> `Select [slot 1] firm` and select the FIRM file of your main CFW. On typical systems that is `smdc:/boot.firm`, but anything goes.
* [optional] Enter `Boot setup...` -> `Change boot mode...` -> `Set quiet boot`. Your system is now set to autoboot and will silently boot the CFW you selected above.

You may also want to set up the other boot slots, and assign key combos to them. Keep in mind you need one autoboot slot (= a slot with a key combo assigned). If you want to access the fastboot3DS menu at a later point in time, hold the HOME button when powering on the console. From the fastboot3DS menu, you may continue the boot process via `Continue boot` or power off the console via the POWER button.

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
* **devkitPro (for the toolchain/makefiles)
* **fincs**, **Al3x_10m**, **Wolfvak**, **Shadowhand**, **Lilith Valentine**, **Crimson**, **Ordim3n** (closed beta testing)
* ...everyone who contributed to **3dbrew.org**

Copyright (C) 2017 derrek, profi200, d0k3
