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
TARGET          = bin/cb_asm_8080

DEFINES        += "QT_MESSAGELOGCONTEXT"

DEPENDPATH     += .
INCLUDEPATH    += src
DESTDIR        += .

OBJECTS_DIR     = build
MOC_DIR         = build
UI_DIR          = build
RCC_DIR         = build

HEADERS        += $$system(ls src/*.h)
SOURCES        += $$system(ls src/*.cpp)

####################################################################################################

# Flex/Bison support of Qt is 'suboptimal'. But following should do in this case.

FLEX_SOURCES                = src/cb_asm_8080.flex
BISON_SOURCES               = src/cb_asm_8080.bison

flex.commands               = flex -o build/cb_asm_8080.flex.cpp ${QMAKE_FILE_IN}
flex.output                 = build/cb_asm_8080.flex.cpp
flex.input                  = FLEX_SOURCES
flex.variable_out           = SOURCES
flex.name                   = flex ${QMAKE_FILE_IN}
flex.depends                = src/cb_unit.h
flex.depends               += build/cb_asm_8080.bison.hpp
QMAKE_EXTRA_COMPILERS      += flex

bisonsource.commands        = bison -d -v -o build/cb_asm_8080.bison.cpp ${QMAKE_FILE_IN}
bisonsource.output          = build/cb_asm_8080.bison.cpp
bisonsource.input           = BISON_SOURCES
bisonsource.variable_out    = SOURCES
bisonsource.name            = bisonsource ${QMAKE_FILE_IN}
bisonsource.depends        += src/cb_unit.h
bisonsource.depends        += src/cb_line.h
QMAKE_EXTRA_COMPILERS      += bisonsource

bisonheader.commands        = bison -d -v -o build/cb_asm_8080.bison.cpp ${QMAKE_FILE_IN}
bisonheader.output          = build/cb_asm_8080.bison.hpp
bisonheader.input           = BISON_SOURCES
bisonheader.variable_out    = HEADERS
bisonheader.name            = bisonheader ${QMAKE_FILE_IN}
bisonheader.depends        += src/cb_unit.h
bisonheader.depends        += src/cb_line.h
QMAKE_EXTRA_COMPILERS      += bisonheader

####################################################################################################

# vim: syntax=qmake ts=4 sw=4 sts=4 sr et columns=100 lines=45
