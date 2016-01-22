#-------------------------------------------------
#
# Project created by QtCreator 2015-08-28T16:11:25
#
#-------------------------------------------------

QT       += widgets

QT       -= gui

CONFIG += c++11

TARGET = StayAwakePoi
TEMPLATE = lib

DEFINES += STAYAWAKEPOI_LIBRARY

SOURCES += poi.cpp\

HEADERS += poi.h\
        stayawakepoi_global.h\

unix {
    target.path = /usr/lib
    INSTALLS += target
}

LIBS += -framework IOKit
LIBS += -framework CoreFoundation
