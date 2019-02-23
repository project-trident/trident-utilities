QT *= network x11extras

message("Using SingleApplication class")

#LUtils Files
SOURCES *= $${PWD}/SingleApplication.cpp
HEADERS *= $${PWD}/SingleApplication.h

INCLUDEPATH *= ${PWD}
