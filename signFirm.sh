#!/bin/bash

dd if=fastboot3DS.firm of=/tmp/fastboot3DS_firm_header.bin bs=256 count=1
openssl dgst -sha256 -sign fastboot3DS_priv.key -out /tmp/fastboot3DS_firm_header.sig /tmp/fastboot3DS_firm_header.bin
dd if=/tmp/fastboot3DS_firm_header.sig of=fastboot3DS.firm bs=256 count=1 seek=1 conv=notrunc
