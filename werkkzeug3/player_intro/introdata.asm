; intro data file

section .data

global	_DebugData

; comment this out for makedemo builds
_DebugData:
	incbin "../data/oceanic_64k.kx"
  ;dw 0