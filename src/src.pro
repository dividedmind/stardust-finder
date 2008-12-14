SOURCES += stardust.cpp \
           main.cpp \
 triview.cpp \
 imagestack.cpp \
 stardustconnector.cpp \
 standardanalyzer.cpp \
 imageanalyzer.cpp
HEADERS += stardust.h \
 triview.h \
 imagestack.h \
 stardustconnector.h \
 standardanalyzer.h \
 imageanalyzer.h
TEMPLATE = app
CONFIG += warn_on \
	  thread \
          qt \
 debug
QT += network
TARGET = ../bin/stardust
RESOURCES = application.qrc
FORMS += triview.ui \
 stardustconfig.ui

QMAKE_CXXFLAGS = ""-Wall -Werror""

DISTFILES += ../TODO
DESTDIR = .

CONFIG -= release

