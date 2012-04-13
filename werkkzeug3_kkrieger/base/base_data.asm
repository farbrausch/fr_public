; This file is distributed under a BSD license. See LICENSE.txt for details.

; data files

                bits    32

                section .data

global  _depacker
global  _depackerSize

global  _depacker2
global  _depacker2Size

global  _Skin05

_depacker       incbin DEPACKER
_depackerSize   dd $-_depacker

_depacker2      incbin DEPACK2
_depacker2Size  dd $-_depacker2


;_Skin05   times 128*1024 dd 0
_Skin05		incbin "../skin05.tga"
