

TEMPLATE = app
#Don't need any Qt - just a simple C program
QT = 
CONFIG += console

TARGET = .susysup
target.path = /usr/local/sbin

SOURCES = main.c

#The binary needs to be setuid root for system access
perms.path=/usr/local/bin
perms.extra="chmod 4555 $(INSTALL_ROOT)/usr/local/sbin/.susysup";

INSTALLS += target perms
