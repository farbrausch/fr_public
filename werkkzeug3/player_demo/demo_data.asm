; This file is distributed under a BSD license. See LICENSE.txt for details.

; data files

                bits    32

                section .data

                global _DebugData
                global _LoaderTune
                global _LoaderTuneSize
                
_DebugData:     ;incbin "../data/debris/debris.kx"
_LoaderTune:    ;incbin "../data/debris/debrisfx_beta_edited.v2m"
_LoaderTuneSize dd $-_LoaderTune
 
