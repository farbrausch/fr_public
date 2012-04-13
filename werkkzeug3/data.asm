; This file is distributed under a BSD license. See LICENSE.txt for details.

; data files

                bits    32

                section .data
                %include "_config.inc"

                global  _Material11vsfr
                global  _Material11psfr
_Material11vsfr incbin  "data/material11.vsfr"                
_Material11psfr incbin  "data/material11.psfr"

                %if     sPLAYER==0 && sMAKEDEMO
                global  _PlayerEXE
_PlayerEXE:     incbin  "player release/werkkzeug3.exe"
                %endif

                %if     sPLAYER && sPROJECT==sPROJ_SNOWBLIND
                global  _Resource
                global  _ResourceSize
_Resource:      incbin  "player release/snowblind-resource.res"
_ResourceSize:  dd      $-_Resource
                %elif   sPLAYER && sPROJECT==sPROJ_DEMO
                global  _Resource
                global  _ResourceSize
_Resource:      incbin  "player release/demo-resource.res"
_ResourceSize:  dd      $-_Resource
                %endif

                %if     sPLAYER && (sDEBUG || (sMAKEDEMO==0))
                %if     sPROJECT==sPROJ_KKRIEGER
                global  _DebugData
_DebugData:     incbin  "data\kkrieger3383.kx"
                %endif
                %if     sPROJECT==sPROJ_SNOWBLIND
                global  _DebugData
_DebugData:     ;incbin  "data\snowblind017.kx"
                %endif
                %if     sPROJECT==sPROJ_DEMO
                global  _DebugData
_DebugData:     ;incbin  "data\bw3_c_debug.kx"
                %endif
                %endif
                


                %if sWERKKZEUG

                global  _depacker
                global  _depackerSize
                global  _depacker2
                global  _depacker2Size
                
_depacker       incbin  sOUTDIR/depacker.bin"
_depackerSize   dd      $-_depacker

_depacker2      incbin  sOUTDIR/depack2.bin"
_depacker2Size  dd      $-_depacker2

                %endif
