# trident-utilities
System configuration utilities for Project Trident

## trident-automount
* Source Directory: src-go/automount
* Runtime Dependencies
   * autofs
   * xdg-open
   * udevadm
* To build:
   * `make`
   * `make install`
* To run it:
   * Enable the "autofs" service: `sudo ln -s /etc/sv/autofs /var/service/`
   * Enable/start the service: `sudo ln -s /etc/sv/trident-automount /var/service/`

### Description
This is a backend daemon for Void Linux systems which monitors udev events and generates XDG desktop shortcuts for removable devices within the "/media" directory. For browsing file-storage devices, *trident-automount* will automatically configure some *autofs* rules which allow these devices to be dynamically browsed via the "/browse/<devicename>" directories. The XDG shortcuts that trident-automount generates will automatically check/adjust the shortcuts based upon the type of device/filesystem.

The devices are mounted on the system by autofs **only while they are being used**. Trident-automount has the autofs mountpoints configured such that any device which is not used for 5 seconds will be automatically un-mounted, and can safely be detached from the system. If you wish to verify if a device is safe to remove, just run "mount" from the command line to ensure that the device is not included at the bottom of the list.

***Mimetypes***

* Audio CD
   * Shortcut uses "xdg-open" to launch the appropriate application for the "cdda" mimetype
* Video CD/DVD/Blueray (*udf* filesystem)
   * Shortcut uses "xdg-open" to launch the appropriate application for the "dvd" mimetype
* Data CD (*cd9660* filesystem)
   * Shortcut uses "xdg-open" to launch a file manager on the appropriate autofs mountpoint
* Data Devices (
   * Shortcut uses "xdg-open" to launch a file manager on the appropriate autofs mountpoint
