#-------------------------------------------------
#
# Project created by QtCreator 2011-10-17T21:44:58
#
#-------------------------------------------------

QT       += core gui svg opengl

TARGET = PatchCanvas
TEMPLATE = app

#CONFIG = debug


SOURCES += main.cpp\
        canvastestapp.cpp \
    patchscene.cpp \
    theme.cpp \
    canvasbox.cpp \
    patchcanvas.cpp \
    canvasportglow.cpp \
    canvasboxshadow.cpp \
    canvasicon.cpp \
    canvasfadeanimation.cpp \
    canvasline.cpp \
    canvasport.cpp \
    canvasbezierline.cpp \
    canvaslinemov.cpp \
    canvasbezierlinemov.cpp

HEADERS  += canvastestapp.h \
    patchscene.h \
    theme.h \
    canvasbox.h \
    patchcanvas.h \
    canvasportglow.h \
    canvasboxshadow.h \
    canvasicon.h \
    patchcanvas-api.h \
    canvasfadeanimation.h \
    canvasline.h \
    canvasport.h \
    canvasbezierline.h \
    canvaslinemov.h \
    canvasbezierlinemov.h

FORMS    += canvastestapp.ui

LIBS += -ljack
