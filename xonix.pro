#-------------------------------------------------
#
# Project created by QtCreator 2014-04-18T13:19:56
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = xonix
TEMPLATE = app


SOURCES += qt/main.cpp    \
           qt/widget.cpp  \
           qt/game.cpp    \
           engine/xonix.c \
           qt/gitem.cpp

HEADERS  += qt/widget.h   \
    qt/game.h             \
    engine/xonix.h        \
    qt/gitem.h

FORMS    += qt/widget.ui
