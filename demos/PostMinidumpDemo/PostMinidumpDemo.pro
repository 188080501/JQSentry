QT += core network concurrent
QT -= gui

CONFIG += c++11

TEMPLATE = app

include( $$PWD/../../library/JQLibrary/JQSentry.pri )

SOURCES += \
    $$PWD/cpp/main.cpp

RESOURCES += \
    $$PWD/data/data.qrc

mac {
    CONFIG += sdk_no_version_check
}
