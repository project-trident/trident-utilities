TEMPLATE = subdirs
CONFIG += recursive

SUBDIRS+= trident-updatemgr \
	susysup

adesk.files = xdg-entry/trident-updatemgr-autostart.desktop
adesk.path = $${INSTALL_ROOT}/usr/local/etc/xdg/autostart

desk.files = xdg-entry/trident-updatemgr.desktop
desk.path = $${INSTALL_ROOT}/usr/local/share/applications

INSTALLS += adesk desk
