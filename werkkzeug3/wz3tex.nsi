; example2.nsi
;
; This script is based on example1.nsi, but it remember the directory, 
; has uninstall support and (optionally) installs start menu shortcuts.
;
; It will install makensisw.exe into a directory that the user selects,

;--------------------------------

SetCompressor lzma
Icon theprodukkt.ico
UninstallIcon theprodukkt.ico

; The name of the installer
Name "Werkkzeug3 Texture Edition"

; The file to write
OutFile "wz3tex_Setup.exe"

; The default installation directory
InstallDir $PROGRAMFILES\Werkkzeug3Tex

; Registry key to check for directory (so if you install again, it will 
; overwrite the old one automatically)
InstallDirRegKey HKLM "Software\Werkkzeug3Tex" "Install_Dir"

;--------------------------------

; Pages

Page directory
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

;--------------------------------

; The stuff to install
Section "Werkkzeug3 Texture Edition"

  SectionIn RO
  
  ; Set output path to the installation directory.
  SetOutPath $INSTDIR
  
  ; Put file there
  File /oname=wz3te.exe "werkkzeug3\release_texture\werkkzeug3.exe" 
  File /oname=texdemo.k "data\examples.k" 
  File "data\simple example.k" 
;  File "data\texdemo.k"
  SetOutPath $INSTDIR\help
  File "data\help\*.*"
  SetOutPath $INSTDIR\help\images
  File "data\help\images\*.*"
  SetOutPath $INSTDIR
  
  CreateDirectory "$INSTDIR\backup"
  CreateDirectory "$INSTDIR\help"
  CreateDirectory "$INSTDIR\help\images"
  
  ; Write the installation path into the registry
  WriteRegStr HKLM SOFTWARE\Werkkzeug1 "Install_Dir" "$INSTDIR"
  
  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Werkkzeug3Tex" "DisplayName" "Werkkzeug3 Texture Edition"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Werkkzeug3Tex" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Werkkzeug3Tex" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Werkkzeug3Tex" "NoRepair" 1
  WriteUninstaller "uninstall.exe"
  
  ; Start-Menu
  CreateDirectory "$SMPROGRAMS\.theprodukkt\Werkkzeug3 Texture Edition"
  CreateShortCut "$SMPROGRAMS\.theprodukkt\Werkkzeug3 Texture Edition\wz3tex.lnk" "$INSTDIR\wz3te.exe" "" "$INSTDIR\wz3te.exe" 0
  CreateShortCut "$SMPROGRAMS\.theprodukkt\Werkkzeug3 Texture Edition\uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
  CreateShortCut "$SMPROGRAMS\.theprodukkt\Werkkzeug3 Texture Edition\big example.lnk" "$INSTDIR\texdemo.k" "" "$INSTDIR\texdemo.k" 0
  CreateShortCut "$SMPROGRAMS\.theprodukkt\Werkkzeug3 Texture Edition\simple example.lnk" "$INSTDIR\simple example.k" "" "$INSTDIR\simple example.k" 0
  CreateShortCut "$SMPROGRAMS\.theprodukkt\Werkkzeug3 Texture Edition\Help.lnk" "$INSTDIR\help\index.html" "" "$INSTDIR\help\index.html" 0

  ; Register Filetypes
  WriteRegStr HKCR ".k" "" "werkkzeug3.project"
  WriteRegStr HKCR "werkkzeug3.project" "" "werkkzeug3 project file"
  WriteRegStr HKCR "werkkzeug3.project\DefaultIcon" "" "$INSTDIR\wz3te.exe,0"
  WriteRegStr HKCR "werkkzeug3.project\shell\open\command" "" '$INSTDIR\wz3te.exe "%1"'

SectionEnd

;--------------------------------

; Uninstaller

Section "Uninstall"
  
  ; Remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Werkkzeug3Tex"
  DeleteRegKey HKLM SOFTWARE\Werkkzeug3Tex
  DeleteRegKey HKCR ".k"
  DeleteRegKey HKCR "werkkzeug3.project"
  
  ; Remove files and uninstaller
  Delete "$INSTDIR\wz3te.exe"
  Delete "$INSTDIR\texdemo.k"
  Delete "$INSTDIR\simple example.k"
  Delete "$INSTDIR\uninstall.exe"
  Delete "$INSTDIR\help\images\*.*"
  Delete "$INSTDIR\help\*.*"

  ; Remove shortcuts, if any
  Delete "$SMPROGRAMS\.theprodukkt\Werkkzeug3 Texture Edition\*.*"

  ; Remove directories used
  RMDir "$SMPROGRAMS\.theprodukkt\Werkkzeug3 Texture Edition"
  RMDir "$SMPROGRAMS\.theprodukkt"
  RMDir "$INSTDIR\backup"
  RMDir "$INSTDIR\help\images"
  RMDir "$INSTDIR\help"
  RMDir "$INSTDIR"

SectionEnd
