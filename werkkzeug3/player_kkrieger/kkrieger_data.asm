; This file is distributed under a BSD license. See LICENSE.txt for details.

; data files

                bits    32

                section .data

                global  _Material11vsfr
                global  _Material11psfr
_Material11vsfr incbin  "../data/material11.vsfr"                
_Material11psfr incbin  "../data/material11.psfr"
