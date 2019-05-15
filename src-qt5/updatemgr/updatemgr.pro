TEMPLATE = subdirs
CONFIG += recursive

SUBDIRS+= trident-updatemgr \
	susysup

adesk.files = xdg-entry/trident-updatemgr-autostart.desktop
adesk.path = $${INSTALL_ROOT}/usr/local/etc/xdg/autostart

desk.files = xdg-entry/trident-updatemgr.desktop
desk.path = $${INSTALL_ROOT}/usr/local/share/applications

script.files = delete_be_multi.sh
script.path = $${INSTALL_ROOT}/usr/local/share/trident/scripts

INSTALLS += adesk desk script
