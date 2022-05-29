QT       += core gui multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    audioplayer.cpp \
    main.cpp \
    mainwidget.cpp \
    mainwindow.cpp \
    networkparser.cpp \
    networkthread.cpp \
    oggdecoder.cpp \
    messages.pb.cc

HEADERS += \
    audioplayer.h \
    mainwidget.h \
    mainwindow.h \
    networkparser.h \
    networkthread.h \
    oggdecoder.h \
    messages.pb.h

FORMS += \
    mainwindow.ui

LIBS += -lopusfile -lopus -lprotobuf
INCLUDEPATH += C:/msys64/mingw64/include/opus

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    stylesheet.txt

RESOURCES += \
    resources.qrc
