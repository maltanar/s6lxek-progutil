#-------------------------------------------------
#
# Project created by QtCreator 2014-04-17T14:13:57
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = s6lxek-progutil
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    datareceivethread.cpp \
    datasendthread.cpp

HEADERS  += mainwindow.h \
    datareceivethread.h \
    datasendthread.h

FORMS    += mainwindow.ui
