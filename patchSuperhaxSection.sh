#!/bin/bash

dd if=/dev/zero of=fastboot3DS.firm bs=4 count=1 seek=30 conv=notrunc
