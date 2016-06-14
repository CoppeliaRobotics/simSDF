QT += core gui widgets

TARGET = v_repExtSDF
TEMPLATE = lib

DEFINES -= UNICODE
DEFINES += QT_COMPIL
CONFIG += shared debug_and_release
INCLUDEPATH += "../include"

*-msvc* {
	QMAKE_CXXFLAGS += -O2
	QMAKE_CXXFLAGS += -W3
}

*-g++*|*clang* {
	QMAKE_CXXFLAGS += -O3 -Wno-unused-parameter
	QMAKE_CFLAGS += -O3 -Wno-unused-parameter
}

win32 {
    DEFINES += WIN_VREP
}

macx {
    DEFINES += MAC_VREP
    INCLUDEPATH += "/usr/local/Cellar/boost/1.60.0_1/include"
    ## libsdformat currently has some problems
    #INCLUDEPATH += "/usr/local/Cellar/sdformat/2.3.2/include/sdformat-2.3"
    #INCLUDEPATH += "/usr/local/Cellar/tinyxml/2.6.2/include"
    #QMAKE_LIBDIR += "/usr/local/Cellar/sdformat/2.3.2/lib"
    #LIBS += "-lsdformat"
}

unix:!macx {
    DEFINES += LIN_VREP
}

unix:!symbian {
    maemo5 {
        target.path = /opt/usr/lib
    } else {
        target.path = /usr/lib
    }
    INSTALLS += target
}

stubs_h.target = stubs.h
stubs_h.output = stubs.h
stubs_h.input = callbacks.xml
stubs_h.commands = python -m v_repStubsGen -H stubs.h callbacks.xml
QMAKE_EXTRA_TARGETS += stubs_h
PRE_TARGETDEPS += stubs.h

stubs_cpp.target = stubs.cpp
stubs_cpp.output = stubs.cpp
stubs_cpp.input = callbacks.xml
stubs_cpp.commands = python -m v_repStubsGen -C stubs.cpp callbacks.xml
QMAKE_EXTRA_TARGETS += stubs_cpp
PRE_TARGETDEPS += stubs.cpp

reference_html.target = reference.html
reference_html.output = reference.html
reference_html.input = callbacks.xml
reference_html.commands = saxon -s:callbacks.xml -a:on -o:reference.html
QMAKE_EXTRA_TARGETS += reference_html
PRE_TARGETDEPS += reference.html

HEADERS += \
    debug.h \
    ../include/v_repLib.h \
    stubs.h \
    tinyxml2.h \
    UIFunctions.h \
    UIProxy.h \
    SDFDialog.h \
    v_repExtSDF.h

SOURCES += \
    debug.cpp \
    ../common/v_repLib.cpp \
    stubs.cpp \
    tinyxml2.cpp \
    UIFunctions.cpp \
    UIProxy.cpp \
    SDFDialog.cpp \
    v_repExtSDF.cpp

FORMS += \
    SDFDialog.ui

