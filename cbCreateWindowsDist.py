#!/usr/bin/env python3
# -*- coding: utf-8 -*-

#;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
#
# $BeginLicense$
#
# $EndLicense$
#
#;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

import sys

if not 'mingw64' in sys.executable:
    print("This script should run the mingw64 python.")
    sys.exit(1)

if sys.version_info[0] != 3 or sys.version_info[1] < 6:
    print("This script requires Python version 3.6.")
    sys.exit(1)

import errno
import os
import re
import shutil
import stat
import subprocess

from time import localtime
from time import strftime

#;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

ProgramName = 'cbEmu8080.exe'

ExtraDirs  = [
        'CharSets', 
        'DiskImages', 
        'MemoryImages', 
        'Themes'
        ]

ExtraFiles = [
        'cbAsm8080/Bin/cbAsm8080.exe',
        'cpmtools/bin/cpmchattr.exe',
        'cpmtools/bin/cpmchmod.exe',
        'cpmtools/bin/cpmcp.exe',
        'cpmtools/bin/cpmls.exe',
        'cpmtools/bin/cpmrm.exe',
        'cpmtools/bin/fsck.cpm.exe',
        'cpmtools/bin/fsed.cpm.exe',
        'cpmtools/bin/mkfs.cpm.exe',
        'cpmtools/bin/diskdefs'
        ]

ThisDir       = os.path.dirname(os.path.realpath(__file__)).replace(os.sep, '/')
TgtDir        = f"{ThisDir}/DistWindows"
SrcExecutable = f"{ThisDir}/bin/cbEmu8080.exe"
TgtExecutable = f"{TgtDir}/cbEmu8080.exe"

FileCmd     = "file"
FindCmd     = "find"
GrepCmd     = "grep"
ObjDump     = "objdump"
WinDeployQt = "WinDeployQt" # part of Qt!

DllBases = ['C:/msys64/mingw64/bin/']
AssumedWindowsDlls = []

#;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

def cbWinDeployQt():

    try:
        OutputLines = os.popen(f"{WinDeployQt} {TgtExecutable}").read().strip().split('\n')
    except Exception as e:
        cbExitMessage(f"Could not run '{WinDeployQt}' on '{TgtExecutable}': {e}.")

    for Line in OutputLines:
        print(f"{WinDeployQt}: {Line}.")

#;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

def cbCollectBins():

    if not os.path.isfile(TgtExecutable):
        cbExitMessage(f"Cannot find '{TgtExecutable}'. Did you build correct ?")

    try:
        FileType = os.popen(f"{FileCmd} {TgtExecutable}").read().strip().split('\n')[0].split(':')[1].strip()
    except Exception as e:
        cbExitMessage(f"Could not run '{FileCmd}' on '{TgtExecutable}': {e}.")

    DllBaseBins = []

    for DllBase in DllBases:

        if not os.path.isdir(DllBase):
            cbExitMessage(f"Cannot find '{DllBase}'. Do you have the DllBase correct ?")

        try:
            DllBaseBins += os.popen(f"{FindCmd} {DllBase} -type d -name 'bin'").read().strip().split('\n')
        except Exception as e:
            cbExitMessage(f"Could not run '{FindCmd}' on '{DllBase}' while looking for bins: {e}.")

    DllMap = {}

    for DllBaseBin in DllBaseBins:

        try:
            FoundDlls = os.popen(f"{FindCmd} {DllBaseBin} -name '*.dll'").read().strip().split('\n')
        except Exception as e:
            cbExitMessage(f"Could not run '{FindCmd}' on '{DllBaseBin}': {e}.")

        for FullPath in FoundDlls:
            BaseName = os.path.basename(FullPath).lower()
            if BaseName in DllMap:
                cbExitMessage(f"Unforeseen: found '{FullPath}' while having it already as '{DllMap[BaseName]}'.")
            DllMap[BaseName] = FullPath

    if False:
        for K,V in DllMap.items():
            print(f"{K0:20}:{V}")

    CollectedFiles = [TgtExecutable]
    FileIdx = 0
    while FileIdx < len(CollectedFiles):
        aFile = CollectedFiles[FileIdx]
        print(f"{ProgramName}: analyzing '{aFile}'.")
        try:
            Dependants = os.popen(f"{ObjDump} -p {aFile} | {GrepCmd} 'DLL Name'").read().strip().split('\n')
        except Exception as e:
            cbExitMessage(f"Could not run '{ObjDump}' on '{aFile}': {e}.")
        for Dependant in Dependants:
            m = re.search(r'DLL Name: (.*\.dll)', Dependant, re.IGNORECASE)
            if not m:
                continue
            DependantDll = m.group(1).lower()
            if not DependantDll in DllMap:
                if DependantDll not in AssumedWindowsDlls:
                    AssumedWindowsDlls.append(DependantDll)
                continue
            FullDependantDll = DllMap[DependantDll]
            if not FullDependantDll in CollectedFiles:
                CollectedFiles.append( FullDependantDll )
        FileIdx += 1

    print('\n')
    print(f"{ProgramName}: following dlls are assumed to be Windows ones. Review carefully on correctness.")
    for Dll in AssumedWindowsDlls:
        print(f"    {Dll}")
    print('\n')

    for aFile in CollectedFiles:
        if aFile == TgtExecutable:
            continue
        try:
            shutil.copy( aFile , TgtDir )
        except IOError as e:
            cbExitMessage(f"Cannot copy '{aFile}' to '{TgtDir}': {e}.")

    print(f"{ProgramName}: successfully collected dll's.")

#;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

def cbCopyOthersToBin():

    '''
    Dirs and other resources that need to be packed with dist.
    '''

    for aDir in ExtraDirs:
        print(f"{ProgramName}: copying '{aDir}'.")
        try:
            shutil.copytree(aDir, os.path.join(TgtDir, aDir))
        except OSError as e:
            cbExitMessage(f"Cannot create: {os.strerror(e.errno)}.")

    for aFile in ExtraFiles:
        print(f"{ProgramName}: copying '{aFile}'")
        (Dummy, ShortFile) = os.path.split(aFile)
        try:
            shutil.copyfile(aFile, os.path.join(TgtDir, ShortFile))
        except OSError as e:
            cbExitMessage(f"Cannot create: {os.strerror(e.errno)}.")

    print(f"{ProgramName}: successfully copied to '{TgtDir}'.")

#;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

def cbCreateAndInitTgtDir():

    print(f"{ProgramName}: creating and initializing '{TgtDir}'.")

    try:
        if os.path.isdir(TgtDir):
            shutil.rmtree(TgtDir)
        os.makedirs(TgtDir)
        shutil.copy(SrcExecutable, TgtDir)
    except IOError as e:
        cbExitMessage(f"Cannot create and initialize '{TgtDir}': {e}.")

#;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

def cbIntroMessage():

    print()
    print(f"This is {ProgramName} for cbEmu8080.")
    print('(C) 2012-2022 Camiel Bouchier <camiel@bouchier.be>.')
    print(f"Running at: {strftime( '%Y-%m-%d %H:%M:%S' , localtime() )}.")
    print()

#;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

def cbExitMessage(Message = None):

    if Message == None:

        print()
        print(f"{ProgramName} succeeded.")
        print()

    else:

        print()
        print(Message)
        print(f"{ProgramName} failed.")
        print()
        sys.exit(1)

#;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

if __name__ == "__main__":

    cbIntroMessage()
    cbCreateAndInitTgtDir()
    cbWinDeployQt()
    cbCollectBins()
    cbCopyOthersToBin()
    cbExitMessage()

#;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

# vim: syntax=python ts=4 sw=4 sts=4 sr et columns=120
