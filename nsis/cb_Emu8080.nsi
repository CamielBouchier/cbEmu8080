;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; $BeginLicense$
;
; $EndLicense$
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; example1.nsi
;
; This script is perhaps one of the simplest NSIs you can make. All of the
; optional settings are left to their default settings. The installer simply
; prompts the user asking them where to install, and drops a copy of "MyProg.exe"
; there.

;--------------------------------

!define ProgName    "cb_emu_8080"
!define LicenseFile "..\license.txt" 
!define RegKey      "Software\${ProgName}"
!define StartMenu   "$SMPROGRAMS\${ProgName}"
!define UnInstall   "uninstall.exe"
!define UnInstKey   "Software\Microsoft\Windows\CurrentVersion\Uninstall\${ProgName}"

XPStyle on

Name       "${ProgName}"
Caption    "${ProgName}"
OutFile    "../${ProgName}_Installer.exe"
InstallDir "$PROGRAMFILES\${ProgName}"

LicenseText   "License"
LicenseData   "${Licensefile}"
DirText       "This will install ${ProgName}"
UninstallText "This will uninstall ${ProgName}"

Page license
Page directory
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

Section
  WriteRegStr HKLM "${RegKey}"    "Install_Dir" "   $INSTDIR"
  WriteRegStr HKLM "${UnInstKey}" "DisplayName"     "${ProgName} (remove only)"
  WriteRegStr HKLM "${uninstkey}" "UninstallString" '"$INSTDIR\${UnInstall}"'
 
; XXX CB TODO
;!ifdef icon
;  WriteRegStr HKCR "${prodname}\DefaultIcon" "" "$INSTDIR\${icon}"
;!endif
 
  SetOutPath $INSTDIR
  File  /r "..\bin"
  File  "..\cb_asm_8080\bin\cb_asm_8080.exe"
  File  "..\cpmtools\bin\*.exe"
  File  "..\cpmtools\bin\diskdefs"
  File  "${LicenseFile}"
  File /r /x ".*" "..\themes"
  File /r /x ".*" "..\memory_images"
  File /r /x ".*" "..\disk_images"
  File /r /x ".*" "..\charsets"
 
; XXX CB TODO 
;!ifdef icon
;File /a "${srcdir}\${icon}"
;!endif
 
  WriteUninstaller "${UnInstall}"
 
SectionEnd
 
Section

  SetOutPath $INSTDIR
  CreateDirectory "${StartMenu}"
  CreateShortCut  "${StartMenu}\${ProgName}.lnk" "$INSTDIR\bin\${ProgName}.exe"

SectionEnd

Section "Uninstall"
  
  DeleteRegKey HKLM "${UnInstKey}"
  DeleteRegKey HKLM "${RegKey}"
 
  Delete "${StartMenu}\*.*"
  Delete "${StartMenu}"
  Delete $INSTDIR\Uninstall.exe
  Delete $INSTDIR\themes\*.*
  Delete $INSTDIR\memory_images\*.*
  Delete $INSTDIR\disk_images\*.*
  Delete $INSTDIR\charsets\*.*
  Delete $INSTDIR\*.*
  RMDir $INSTDIR\themes
  RMDir $INSTDIR\memory_images
  RMDir $INSTDIR\disk_images
  RMDir $INSTDIR\charsets
  RMDir $INSTDIR

SectionEnd 

; vim: syntax=nsis ts=4 sw=4 sts=4 sr et columns=100 lines=45 fileencoding=utf-8
