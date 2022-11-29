QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

DEFINES += __LINUX_ALSA__
CONFIG += c++17

LIBS += -L/home/ilia/rpi-qt/sysroot/usr/include/alsa/asoundlib.h -lasound\
-L/home/ilia/rpi-qt/sysroot/usr/include/pthread.h -lpthread

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    MidiCallbacks.cpp \
    RtMidi.cpp \
    bcm2835.c \
    main.cpp \
    mainwindow.cpp \
    midiprocess.cpp \
    mygpio.cpp \
    st7789.cpp

HEADERS += \
    DisplayThread.h \
    MidiCallbacks.h \
    RtMidi.h \
    bcm2835.h \
    mainwindow.h \
    midiprocess.h \
    mygpio.h \
    st7789.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /home/pi/$${TARGET}
!isEmpty(target.path): INSTALLS += target