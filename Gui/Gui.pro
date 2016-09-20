#-------------------------------------------------
#
# Project created by QtCreator 2016-06-30T17:44:13
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
TARGET   = Gui
TEMPLATE = app
ICON     = AppIcon.icns

# Output directory
CONFIG(debug, debug|release) {
    output = debug
}
CONFIG(release, debug|release) {
    output = release
}

DESTDIR     = bin
OBJECTS_DIR = $$output
MOC_DIR     = $$output
RCC_DIR     = $$output
UI_DIR      = $$output

SOURCES += main.cpp\
        widget.cpp \
    FramelessWindow.cpp \
    MyThread.cpp \
    HttpClient.cpp \
    Bobo.cpp

HEADERS  += widget.h \
    FramelessWindow.h \
    MyThread.h \
    HttpClient.h \
    Bobo.h

FORMS    += widget.ui
