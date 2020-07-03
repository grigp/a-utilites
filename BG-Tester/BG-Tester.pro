#-------------------------------------------------
#
# Project created by QtCreator 2020-02-07T09:38:55
#
#-------------------------------------------------

QT       += core gui serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = BG-Tester
CONFIG += c++11
TEMPLATE = app

# каталог, в котором будет располагаться результирующий исполняемый файл
DESTDIR = $$OUT_PWD/bin

MOC_DIR = moc
OBJECTS_DIR = obj
RCC_DIR = rcc
UI_DIR = uic


# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

include(../common/ports/ports_src.pri)
include(driver/driver_src.pri)


SOURCES += \
        main.cpp \
        mainwindow.cpp \
    editsettingsdialog.cpp

HEADERS += \
        mainwindow.h \
    editsettingsdialog.h

FORMS += \
        mainwindow.ui \
    editsettingsdialog.ui

RESOURCES += \
    bg-tester.qrc

win32:RC_FILE = bg-tester.rc

DISTFILES +=


