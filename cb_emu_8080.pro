####################################################################################################
#
# $BeginLicense$
#
# $EndLicense$
#
####################################################################################################

CONFIG          -= debug_and_release
CONFIG          -= debug
CONFIG          += release
CONFIG          += console # windows  # console provides much feedback we need !

QT              += widgets

TEMPLATE        = app
TARGET          = bin/cb_emu_8080

DEFINES         += "CB_WITH_VTTEST"
DEFINES         += "QT_MESSAGELOGCONTEXT"

DEPENDPATH      += .
INCLUDEPATH     += src
INCLUDEPATH     += src/vt_test
DESTDIR         += .

OBJECTS_DIR     = build
MOC_DIR         = build
UI_DIR          = build
RCC_DIR         = build

QMAKE_CXXFLAGS  += -save-temps=obj
QMAKE_CXXFLAGS  += -Werror

HEADERS         += $$system(ls src/*.h)
HEADERS         += $$system(ls src/vt_test/*.h)
FORMS           += $$system(ls src/ui/*.ui)
SOURCES         += $$system(ls src/*.cpp)
SOURCES         += $$system(ls src/vt_test/*.cpp)
RESOURCES        = src/cb_emu_8080.qrc

RC_ICONS         = icons/cb_emu_8080.ico

####################################################################################################

# vim: syntax=qmake ts=4 sw=4 sts=4 sr et columns=100 lines=45
