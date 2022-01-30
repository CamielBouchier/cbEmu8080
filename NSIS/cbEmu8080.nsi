;~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
;
; $BeginLicense$
;
; $EndLicense$
;
;~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

!define ProgName    "cbEmu8080"
!define LicenseFile "..\License.txt" 
!define RegKey      "Software\${ProgName}"
!define StartMenu   "$SMPROGRAMS\${ProgName}"
!define UnInstall   "uninstall.exe"
!define UnInstKey   "Software\Microsoft\Windows\CurrentVersion\Uninstall\${ProgName}"

XPStyle on

Name       "${ProgName}"
Caption    "${ProgName}"
OutFile    "../${ProgName}Installer.exe"
InstallDir "$PROGRAMFILES64\${ProgName}"

LicenseText   "License"
LicenseData   "${LicenseFile}"
DirText       "This will install ${ProgName}"
UninstallText "This will uninstall ${ProgName}"

;~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Page license
Page directory
Page instfiles

;~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

UninstPage uninstConfirm
UninstPage instfiles

;~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Section

  WriteRegStr HKLM "${RegKey}"    "Install_Dir" "   $INSTDIR"
  WriteRegStr HKLM "${UnInstKey}" "DisplayName"     "${ProgName} (remove only)"
  WriteRegStr HKLM "${uninstkey}" "UninstallString" '"$INSTDIR\${UnInstall}"'
 
  SetOutPath $INSTDIR

  File  "${LicenseFile}"

  File /r /x ".*" "..\DistWindows\*"
  File /r /x ".*" "..\DistWindows\*.*"

  WriteUninstaller "${UnInstall}"
 
SectionEnd
 
;~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Section

  SetOutPath $INSTDIR
  CreateDirectory "${StartMenu}"
  CreateShortCut  "${StartMenu}\${ProgName}.lnk" "$INSTDIR\${ProgName}.exe"

SectionEnd

;~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Section "Uninstall"
  
  DeleteRegKey HKLM "${UnInstKey}"
  DeleteRegKey HKLM "${RegKey}"
 
  Delete "${StartMenu}\*.*"
  Delete "${StartMenu}"
  RMDIR /r $INSTDIR

SectionEnd 

;~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

; vim: syntax=nsis ts=4 sw=4 sts=4 sr et columns=100 lines=45 fileencoding=utf-8
