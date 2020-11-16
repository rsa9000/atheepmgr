atheepmgr: Atheros EEPROM manager
=================================

This is a utility to dump and update the EEPROM content of Atheros based wireless NICs.

atheepmgr supports several operations such as parsing and printing the EEPROM content, updating the EEPROM contents and so on. The utility can access EEPROM content of a real NIC and can works with EEPROM data that has been dumped to the file. You can find a few usage examples below.

The following EEPROM content access techniques are supported:
* via mapping of the PCI device I/O region (using libpciaccess)
* via mapping of the directly specified device I/O region
* file dump

The following OS(s) are tested/supported:

* FreeBSD
* GNU/Linux
* OpenBSD

Warning
-------

Using the utility may be potentially dangerous for your hardware and may cause a local laws violation. The utility is intended for use by qualified software developers and (or) radio engineers. So the

DISCLAIMER
----------

The author(s) are in no case responsible for damaged hardware or violation of local laws by operating modified hardware.

Building
--------

Utility building as simple as typing `make` (or `gmake` on BSD platforms). Builing requirements:
* gcc
* GNU make
* pkg-config (optional, used only to build with libpciaccess support)
* libpciaccess (optional, allows accessing PCI devices by specifing its location, e.g. bus and device numbers)

Usage examples
--------------

### Print EEPROM content of PCI device

Accessing the device which was specified by its address at PCI bus is probably the most simple accessing method.

Example: to print the EEPROM content of a wireless NIC, which is located at PCI bus #1 device #3 just run:

```
# atheepmgr -P 1:3
```

*NB*: to access a hardware as a PCI device you should build the utility with the libpciaccess library support.

### Print EEPROM content of device with known I/O region

Accessing the device's EEPROM by directly specifying the device I/O memory location can be useful for embedded platforms where using the libpciaccess library could be an overkill.

Example: print EEPROM content of the device, which I/O region starts at 0x21000000:

```
# atheepmgr -M 0x21000000
```

*NB*: To obtain the device I/O region location you can examine the output of the `dmesg(1)` command (or check the /proc/iomem file on GNU/Linux based platforms) .

### Parse and print contents of an EEPROM file dump

Useful option for (re-) checking NIC configuration without really attaching it to a host.

Example: print data from a EEPROM dump in file eep.bin, which was extracted earlier from a AR9220 based wireless NIC:

```
# atheepmgr -t 5416 -F eep.bin
```

or using chip name instead of EEPROM map (layout) name:

```
# atheepmgr -t AR9220 -F eep.bin
```

*NB*: chip autodetection is not supported for file access, so you should specifiy EEPROM map (layout) or chip name manually. To see a full list of supported EEPROM maps use a *-h* option.

### Dump NIC EEPROM content to the file

Example: preserve a wireless NIC EEPROM content to the eep.bin file

```
# atheepmgr -M 0x21000000 save eep.bin
```

TODO
----

* Make the utility more scripts-friendly by adding an option to print EEPROM content in a more structured format
* Add a support for loading a file content to the NIC EEPROM to restore somewhere corrupted EEPROM data
* Add a support for automatically enable and wake-up the chip if it not yet active (e.g. if driver is not loaded, or if network interface is DOWN)
* Add option to modify RfSilent settings

EEPROM info sources
-------------------

* Linux kernel driver for AR5xxx chips: https://wireless.wiki.kernel.org/en/users/drivers/ath5k
* Linux kernel driver for AR9xxx chips: https://wireless.wiki.kernel.org/en/users/drivers/ath9k
* Legacy Atheros HAL (for AR5xxx chips): https://github.com/qca/qca-legacy-hal
* ath_info utility from MadWifi project: http://madwifi-project/svn/ath_info and https://github.com/mickflemm/ath-info

History
-------

Originally based on the edump utility from https://github.com/mcgrof/qca-swiss-army-knife project and then hardly reworked to add support for legacy chips, EEPROM modification, file dumps and so on.

License
-------

This project is licensed under the terms of the ISC license. See the LICENSE file for license rights and limitations.
