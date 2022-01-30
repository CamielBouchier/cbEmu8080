####################################################################################################
#
# $BeginLicense$
#
# $EndLicense$
#
####################################################################################################

CONFIG         -= debug_and_release
CONFIG         -= debug
CONFIG         += debug
CONFIG         += console 

TEMPLATE        = app
TARGET          = Bin/cbAsm8080

DEFINES        += "QT_MESSAGELOGCONTEXT"

DEPENDPATH     += .
INCLUDEPATH    += Sources
DESTDIR        += .

OBJECTS_DIR     = Build
MOC_DIR         = Build
UI_DIR          = Build
RCC_DIR         = Build

HEADERS        += $$system(ls Sources/*.h)
SOURCES        += $$system(ls Sources/*.cpp)

####################################################################################################

# Flex/Bison support of Qt is 'suboptimal'. But following should do in this case.

FLEX_SOURCES                = Sources/cbAsm8080.flex
BISON_SOURCES               = Sources/cbAsm8080.bison

flex.commands               = flex -o Build/cbAsm8080.flex.cpp ${QMAKE_FILE_IN}
flex.output                 = Build/cbAsm8080.flex.cpp
flex.input                  = FLEX_SOURCES
flex.variable_out           = SOURCES
flex.name                   = flex ${QMAKE_FILE_IN}
flex.depends                = Sources/cbUnit.h
flex.depends               += Build/cbAsm8080.bison.hpp
QMAKE_EXTRA_COMPILERS      += flex

bisonsource.commands        = bison -d -v -o Build/cbAsm8080.bison.cpp ${QMAKE_FILE_IN}
bisonsource.output          = Build/cbAsm8080.bison.cpp
bisonsource.input           = BISON_SOURCES
bisonsource.variable_out    = SOURCES
bisonsource.name            = bisonsource ${QMAKE_FILE_IN}
bisonsource.depends        += Sources/cbUnit.h
bisonsource.depends        += Sources/cbLine.h
QMAKE_EXTRA_COMPILERS      += bisonsource

bisonheader.commands        = bison -d -v -o Build/cbAsm8080.bison.cpp ${QMAKE_FILE_IN}
bisonheader.output          = Build/cbAsm8080.bison.hpp
bisonheader.input           = BISON_SOURCES
bisonheader.variable_out    = HEADERS
bisonheader.name            = bisonheader ${QMAKE_FILE_IN}
bisonheader.depends        += Sources/cbUnit.h
bisonheader.depends        += Sources/cbLine.h
QMAKE_EXTRA_COMPILERS      += bisonheader

####################################################################################################

# vim: syntax=qmake ts=4 sw=4 sts=4 sr et columns=100 lines=45
