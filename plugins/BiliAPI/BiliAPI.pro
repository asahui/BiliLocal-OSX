#-------------------------------------------------
#
# Project created by QtCreator 2015-08-28T16:26:23
#
#-------------------------------------------------

QT       += widgets network xml

QT       -= gui

CONFIG += c++11

TARGET = BiliAPI
TEMPLATE = lib

DEFINES += BILIAPI_LIBRARY

SOURCES += biliapi.cpp

HEADERS += biliapi.h\
        biliapi_global.h


unix {
    target.path = /usr/lib
    INSTALLS += target
}
