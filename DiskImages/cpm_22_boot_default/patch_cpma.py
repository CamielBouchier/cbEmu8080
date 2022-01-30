# -*- coding: utf-8 -*-

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#
# $BeginLicense$
#
# $EndLicense$
#
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

with open("source_disk/cpma.cpm", "rb") as CpmIn:
    with open("boot_record.bin", "rb") as BootIn:
        with open("bios.bin", "rb") as BiosIn:
            with open("cpm_22_boot.cpm", "wb") as OutFile :
                Pos  = 0
                Byte = CpmIn.read(1)
                while Byte :
                    if Pos < 0x80 :
                        B = BootIn.read(1)
                        if B :
                            Byte = B
                    if Pos >= (0x80 + 0x1600) and Pos < (0x80 + 0x1600 + 0x250) :
                        B = BiosIn.read(1)
                        if B :
                            Byte = B
                    OutFile.write(Byte)
                    Pos  = Pos + 1
                    Byte = CpmIn.read(1)


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# vim: syntax=python ts=4 sw=4 sts=4 sr et columns=100 lines=45 fileencoding=utf-8
