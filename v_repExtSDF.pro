QT += core gui widgets

TARGET = v_repExtSDF
TEMPLATE = lib

DEFINES -= UNICODE
DEFINES += QT_COMPIL
CONFIG += shared debug_and_release
INCLUDEPATH += "../include"
INCLUDEPATH += "../v_repMath"

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
    ../v_repMath/3Vector.h \
    ../v_repMath/3X3Matrix.h \
    ../v_repMath/4Vector.h \
    ../v_repMath/4X4FullMatrix.h \
    ../v_repMath/4X4Matrix.h \
    ../v_repMath/6Vector.h \
    ../v_repMath/6X6Matrix.h \
    ../v_repMath/7Vector.h \
    ../v_repMath/MMatrix.h \
    ../v_repMath/MyMath.h \
    ../v_repMath/VPoint.h \
    ../v_repMath/Vector.h \
    ../v_repMath/mathDefines.h \
    stubs.h \
    tinyxml2.h \
    SDFParser.h \
    UIFunctions.h \
    UIProxy.h \
    ImportOptions.h \
    SDFDialog.h \
    v_repExtSDF.h

SOURCES += \
    debug.cpp \
    ../common/v_repLib.cpp \
    ../v_repMath/3Vector.cpp \
    ../v_repMath/3X3Matrix.cpp \
    ../v_repMath/4Vector.cpp \
    ../v_repMath/4X4FullMatrix.cpp \
    ../v_repMath/4X4Matrix.cpp \
    ../v_repMath/6Vector.cpp \
    ../v_repMath/6X6Matrix.cpp \
    ../v_repMath/7Vector.cpp \
    ../v_repMath/MMatrix.cpp \
    ../v_repMath/MyMath.cpp \
    ../v_repMath/Vector.cpp \
    stubs.cpp \
    tinyxml2.cpp \
    SDFParser.cpp \
    UIFunctions.cpp \
    UIProxy.cpp \
    ImportOptions.cpp \
    SDFDialog.cpp \
    v_repExtSDF.cpp

FORMS += \
    SDFDialog.ui

