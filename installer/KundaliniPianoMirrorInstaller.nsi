; Script generated with the Venis Install Wizard

; Define your application name
!define APPNAME "Kundalini Piano Mirror"
!define APPNAMEANDVERSION "Kundalini Piano Mirror 1.3"

; Main Install settings
Name "${APPNAMEANDVERSION}"
InstallDir "$PROGRAMFILES\Kundalini Piano Mirror"
InstallDirRegKey HKLM "Software\${APPNAME}" ""
OutFile "KundaliniPianoMirror-x86-1.3.exe"

; Modern interface settings
!include "MUI.nsh"

!define MUI_ABORTWARNING
!define MUI_FINISHPAGE_RUN "$INSTDIR\PianoMirror.exe"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

; Set languages (first is default language)
!insertmacro MUI_LANGUAGE "English"
!insertmacro MUI_RESERVEFILE_LANGDLL

Section "Kundalini Piano Mirror" Section1

	; Set Section properties
	SetOverwrite on

	; Set Section Files and Shortcuts
	SetOutPath "$INSTDIR\"
	File "..\Release\PianoMirror.exe"
	File "..\README.md"
	
	CreateShortCut "$DESKTOP\Kundalini Piano Mirror.lnk" "$INSTDIR\PianoMirror.exe"
	CreateDirectory "$SMPROGRAMS\Kundalini Piano Mirror"
	CreateShortCut "$SMPROGRAMS\Kundalini Piano Mirror\Kundalini Piano Mirror.lnk" "$INSTDIR\PianoMirror.exe"
	CreateShortCut "$SMPROGRAMS\Kundalini Piano Mirror\Uninstall.lnk" "$INSTDIR\uninstall.exe"

SectionEnd

Section -FinishSection

	WriteRegStr HKLM "Software\${APPNAME}" "" "$INSTDIR"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "DisplayName" "${APPNAME}"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "UninstallString" "$INSTDIR\uninstall.exe"
	WriteUninstaller "$INSTDIR\uninstall.exe"

SectionEnd

; Modern install component descriptions
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
	!insertmacro MUI_DESCRIPTION_TEXT ${Section1} ""
!insertmacro MUI_FUNCTION_DESCRIPTION_END

;Uninstall section
Section Uninstall

	;Remove from registry...
	DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}"
	DeleteRegKey HKLM "SOFTWARE\${APPNAME}"

	; Delete self
	Delete "$INSTDIR\uninstall.exe"

	; Delete Shortcuts
	Delete "$DESKTOP\Kundalini Piano Mirror.lnk"
	Delete "$SMPROGRAMS\Kundalini Piano Mirror\Kundalini Piano Mirror.lnk"
	Delete "$SMPROGRAMS\Kundalini Piano Mirror\Uninstall.lnk"

	; Clean up Kundalini Piano Mirror
	Delete "$INSTDIR\PianoMirror.exe"

	; Remove remaining directories
	RMDir "$SMPROGRAMS\Kundalini Piano Mirror"
	RMDir "$INSTDIR\"

SectionEnd

; eof