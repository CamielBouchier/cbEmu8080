# -*- coding: utf-8 -*-

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#
# $BeginLicense$
#
# $EndLicense$
#
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

with open("source_disk/cpma.cpm", "rb") as InFile:
    with open("cpm_22.bin", "wb") as OutFile :
        Pos  = 0
        Byte = InFile.read(1)
        while Byte :
            if Pos >= 0x80 and Pos < 0x1680 :
                OutFile.write(Byte)
            Pos  = Pos + 1
            Byte = InFile.read(1)

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# vim: syntax=python ts=4 sw=4 sts=4 sr et columns=100 lines=45 fileencoding=utf-8
