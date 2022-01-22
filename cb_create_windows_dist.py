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

program_name = 'cb_emu_8080.exe'

extra_dirs  = [
        'charsets', 
        'disk_images', 
        'memory_images', 
        'themes'
        ]

extra_files = [
        'cb_asm_8080/bin/cb_asm_8080.exe',
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

this_dir       = os.path.dirname(os.path.realpath(__file__)).replace(os.sep, '/')
tgt_dir        = f"{this_dir}/dist_windows"
src_executable = f"{this_dir}/bin/cb_emu_8080.exe"
tgt_executable = f"{tgt_dir}/cb_emu_8080.exe"

file_cmd    = "file"
find_cmd    = "find"
grep_cmd    = "grep"
obj_dump    = "objdump"
windeployqt = "windeployqt" # part of Qt!

dll_bases = ['C:/msys64/mingw64/bin/']
assumed_windows_dlls = []

#;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

def cb_win_deploy_qt():

    try:
        output_lines = os.popen(f"{windeployqt} {tgt_executable}").read().strip().split('\n')
    except Exception as e:
        cb_exit_message(f"Could not run '{windeployqt}' on '{tgt_executable}': {e}.")

    for line in output_lines:
        print(f"{windeployqt}: {line}.")

#;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

def cb_collect_bins():

    if not os.path.isfile(tgt_executable):
        cb_exit_message(f"Cannot find '{tgt_executable}'. Did you build correct ?")

    try:
        file_type = os.popen(f"{file_cmd} {tgt_executable}").read().strip().split('\n')[0].split(':')[1].strip()
    except Exception as e:
        cb_exit_message(f"Could not run '{file_cmd}' on '{tgt_executable}': {e}.")

    dll_base_bins = []

    for dll_base in dll_bases:

        if not os.path.isdir(dll_base):
            cb_exit_message(f"Cannot find '{dll_base}'. Do you have the dll_base correct ?")

        try:
            dll_base_bins += os.popen(f"{find_cmd} {dll_base} -type d -name 'bin'").read().strip().split('\n')
        except Exception as e:
            cb_exit_message(f"Could not run '{find_cmd}' on '{dll_base}' while looking for bins: {e}.")

    dll_map = {}

    for dll_base_bin in dll_base_bins:

        try:
            found_dlls = os.popen(f"{find_cmd} {dll_base_bin} -name '*.dll'").read().strip().split('\n')
        except Exception as e:
            cb_exit_message(f"Could not run '{find_cmd}' on '{dll_base_bin}': {e}.")

        for full_path in found_dlls:
            basename = os.path.basename(full_path).lower()
            if basename in dll_map:
                cb_exit_message(f"Unforeseen: found '{full_path}' while having it already as '{dll_map[basename]}'.")
            dll_map[basename] = full_path

    if False:
        for K,V in dll_map.items():
            print(f"{K0:20}:{V}")

    collected_files = [tgt_executable]
    file_idx = 0
    while file_idx < len(collected_files):
        afile = collected_files[file_idx]
        print(f"{program_name}: analyzing '{afile}'.")
        try:
            dependants = os.popen(f"{obj_dump} -p {afile} | {grep_cmd} 'DLL Name'").read().strip().split('\n')
        except Exception as e:
            cb_exit_message(f"Could not run '{obj_dump}' on '{afile}': {e}.")
        for dependant in dependants:
            m = re.search(r'DLL Name: (.*\.dll)', dependant, re.IGNORECASE)
            if not m:
                continue
            dependant_dll = m.group(1).lower()
            if not dependant_dll in dll_map:
                if dependant_dll not in assumed_windows_dlls:
                    assumed_windows_dlls.append(dependant_dll)
                continue
            full_dependant_dll = dll_map[dependant_dll]
            if not full_dependant_dll in collected_files:
                collected_files.append( full_dependant_dll )
        file_idx += 1

    print('\n')
    print(f"{program_name}: following dlls are assumed to be Windows ones. Review carefully on correctness.")
    for dll in assumed_windows_dlls:
        print(f"    {dll}")
    print('\n')

    for afile in collected_files:
        if afile == tgt_executable:
            continue
        try:
            shutil.copy( afile , tgt_dir )
        except IOError as e:
            cb_exit_message(f"Cannot copy '{afile}' to '{tgt_dir}': {e}.")

    print(f"{program_name}: successfully collected dll's.")

#;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

def cb_copy_others_to_bin():

    '''
    Dirs and other resources that need to be packed with dist.
    '''

    for adir in extra_dirs:
        print(f"{program_name}: copying '{adir}'.")
        try:
            shutil.copytree(adir, os.path.join(tgt_dir, adir))
        except OSError as e:
            cb_exit_message(f"Cannot create: {os.strerror(e.errno)}.")

    for afile in extra_files:
        print(f"{program_name}: copying '{afile}'")
        (dummy, short_afile) = os.path.split(afile)
        try:
            shutil.copyfile(afile, os.path.join(tgt_dir, short_afile))
        except OSError as e:
            cb_exit_message(f"Cannot create: {os.strerror(e.errno)}.")

    print(f"{program_name}: successfully copied to '{tgt_dir}'.")

#;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

def cb_create_and_init_tgt_dir():

    print(f"{program_name}: creating and initializing '{tgt_dir}'.")

    try:
        if os.path.isdir(tgt_dir):
            shutil.rmtree(tgt_dir)
        os.makedirs(tgt_dir)
        shutil.copy(src_executable, tgt_dir)
    except IOError as e:
        cb_exit_message(f"Cannot create and initialize '{tgt_dir}': {e}.")

#;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

def cb_intro_message():

    print()
    print(f"This is {program_name} for cb_emu_8080.")
    print('(C) 2012-2022 Camiel Bouchier <camiel@bouchier.be>.')
    print(f"Running at: {strftime( '%Y-%m-%d %H:%M:%S' , localtime() )}.")
    print()

#;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

def cb_exit_message(message = None):

    if message == None:

        print()
        print(f"{program_name} succeeded.")
        print()

    else:

        print()
        print(message)
        print(f"{program_name} failed.")
        print()
        sys.exit(1)

#;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

if __name__ == "__main__":

    cb_intro_message()
    cb_create_and_init_tgt_dir()
    cb_win_deploy_qt()
    cb_collect_bins()
    cb_copy_others_to_bin()
    cb_exit_message()

#;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

# vim: syntax=python ts=4 sw=4 sts=4 sr et columns=120
