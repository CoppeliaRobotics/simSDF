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
    QMAKE_CXXFLAGS_RELEASE += -O3
    QMAKE_CXXFLAGS_DEBUG += -O0 -g
    QMAKE_CFLAGS_RELEASE += -O3
    QMAKE_CFLAGS_DEBUG += -O0 -g
}

win32 {
    INCLUDEPATH += "c:/boost_1_54_0"
    LIBS += "c:/local/boost_1_60_0/stage/lib/libboost_filesystem-mgw49-mt-1_60.a"
    LIBS += "c:/local/boost_1_60_0/stage/lib/libboost_system-mgw49-mt-1_60.a"
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
    LIBS += "-L/usr/local/lib"
    LIBS += "-lboost_filesystem"
    LIBS += "-lboost_system"
}

unix:!macx {
    LIBS += -lboost_filesystem
    LIBS += -lboost_system
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

gen_all.target = generated/stubs.h
gen_all.output = generated/stubs.h
gen_all.input = callbacks.xml simExtCustomUI.lua
gen_all.commands = python \"$$PWD/external/v_repStubsGen/generate.py\" --xml-file \"$$PWD/callbacks.xml\" --gen-all \"$$PWD/generated/\"
QMAKE_EXTRA_TARGETS += gen_all
PRE_TARGETDEPS += generated/stubs.h

HEADERS += \
    plugin.h \
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

