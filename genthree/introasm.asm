		section		.data

		global		_BabeMesh
_BabeMesh:
    incbin    "data\babe.mesh"
    
    global    _V2MTune
    global    _V2MEnd
_V2MTune:
    incbin    "data\josie.v2m"
_V2MEnd:
    
    global    _Script 
_Script: 
;    incbin    "data\default.raw"
 
    global    _ScriptPack
_ScriptPack: 
    incbin    "data\candytron_final_064.pac"
     
         