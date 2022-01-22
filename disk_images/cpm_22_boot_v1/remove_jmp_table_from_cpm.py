# -*- coding: utf-8 -*-

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#
# $BeginLicense$
#
# $EndLicense$
#
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

import os

with open("cpm_22.bin", "rb") as InFile:
    with open("cpm_22.bin.shortened", "wb") as OutFile :
        Pos  = 0
        Byte = InFile.read(1)
        while Byte :
            if Pos < 0x1600 :
                OutFile.write(Byte)
            Pos  = Pos + 1
            Byte = InFile.read(1)

os.remove("cpm_22.bin")
os.rename("cpm_22.bin.shortened", "cpm_22.bin")


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# vim: syntax=python ts=4 sw=4 sts=4 sr et columns=100 lines=45 fileencoding=utf-8
