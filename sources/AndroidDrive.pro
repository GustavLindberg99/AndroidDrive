QT += widgets

CONFIG += c++17

LIBS *= -luser32

!*-msvc{
    CONFIG += warn_off
    QMAKE_CXXFLAGS += -Wall -Wno-switch-enum -Wno-switch
}

SOURCES += main.cpp \
    androiddevice.cpp \
    devicelistwindow.cpp

RESOURCES += \
    resource.qrc

HEADERS += \
    androiddevice.h \
    devicelistwindow.h \
    programinfo.h

win32:RC_FILE = resource.rc

DISTFILES += \
    resource.rc
