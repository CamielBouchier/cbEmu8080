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
TARGET          = Bin/cbEmu8080

DEFINES         += "CB_WITH_VTTEST"
DEFINES         += "QT_MESSAGELOGCONTEXT"

DEPENDPATH      += .
INCLUDEPATH     += Sources
INCLUDEPATH     += Sources/VtTest
DESTDIR         += .

OBJECTS_DIR     = Build
MOC_DIR         = Build
UI_DIR          = Build
RCC_DIR         = Build

QMAKE_CXXFLAGS  += -save-temps=obj
QMAKE_CXXFLAGS  += -Werror

HEADERS         += $$system(ls Sources/*.h)
HEADERS         += $$system(ls Sources/VtTest/*.h)
FORMS           += $$system(ls Sources/ui/*.ui)
SOURCES         += $$system(ls Sources/*.cpp)
SOURCES         += $$system(ls Sources/VtTest/*.cpp)
RESOURCES        = Sources/cbEmu8080.qrc

RC_ICONS         = icons/cbEmu8080.ico

####################################################################################################

# vim: syntax=qmake ts=4 sw=4 sts=4 sr et columns=100 lines=45
