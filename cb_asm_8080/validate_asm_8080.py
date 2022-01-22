# -*- coding: utf-8 -*-

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#
# $BeginLicense$
#
# $EndLicense$
#
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

import glob
import os
import subprocess

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

RefAssembler = "../reference_material/asm8080/bin/asm8080.exe"
cb_8080_asm    = "bin/cb_asm_8080.exe"

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

print("Checking existence of reference hex files.")

AssemblerFiles = glob.glob("test_cases/*.asm")
for File in AssemblerFiles :
    HexFile = File.replace('.asm', '.hex')
    os.remove(HexFile)
    print("{} {}".format(RefAssembler, File))
    subprocess.run([RefAssembler, File, "-l"])

BinFiles = glob.glob("test_cases/*.bin")
for File in BinFiles :
    print("Removing {}".format(File))
    os.remove(File)

MFiles = glob.glob("*.m")
for File in MFiles :
    print("Removing {}".format(File))
    os.remove(File)

print("Start the checking.")

for File in AssemblerFiles :

    print("Checking {}".format(cb_8080_asm))

    RV = subprocess.run([cb_8080_asm, "-o", "build", "-l", "-a", File])
    if RV.returncode :
        print("Test FAILED for {}.".format(File))
        exit()

    # Compare hex files.
    ShouldHexFile = File.replace('.asm', '.hex')
    OverrideHexFile = File.replace('test_cases', 'test_cases\overrides').replace('.asm', '.hex')
    if (os.path.isfile(OverrideHexFile)) :
        ShouldHexFile = OverrideHexFile
    IsHexFile = File.replace('test_cases', 'build').replace('.asm', '.hex')
    RV = subprocess.run(['diff', IsHexFile , ShouldHexFile])
    if RV.returncode :
        print("Test FAILED for {}. (hex differ)".format(File))
        exit()
    else :
        print("Test OK for {}.".format(File))


#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# vim: syntax=python ts=4 sw=4 sts=4 sr et columns=100 lines=45 fileencoding=utf-8
