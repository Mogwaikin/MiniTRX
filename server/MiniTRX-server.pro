TARGET = MiniTRX-server
QT += core websockets
QT -= gui
CONFIG += static console
TEMPLATE = app
QMAKE_CFLAGS = -ffast-math
INCLUDEPATH += ../wdsp /opt/fftw/fftw-3.2.2-armhf/include /opt/libsamplerate/libsamplerate-0.1.8-armhf/include
LIBS += -L../wdsp -lwdsp -L/opt/fftw/fftw-3.2.2-armhf/lib -lfftw3f -L/opt/libsamplerate/libsamplerate-0.1.8-armhf/lib -lsamplerate
QMAKE_LFLAGS += -static
OBJECTS_DIR = build
MOC_DIR = build
RCC_DIR = build
HEADERS = server.h
SOURCES = server.cpp main.cpp
