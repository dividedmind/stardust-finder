SOURCES += stardust.cpp \
           main.cpp \
 triview.cpp \
 imagestack.cpp \
 stardustconnector.cpp
HEADERS += stardust.h \
 triview.h \
 imagestack.h \
 stardustconnector.h
TEMPLATE = app
CONFIG += debug warn_on \
	  thread \
          qt
QT += network
TARGET = ../bin/stardust
RESOURCES = application.qrc
FORMS += triview.ui \
 stardustconfig.ui

QMAKE_CXXFLAGS = -Wall -Werror

DISTFILES += ../TODO
