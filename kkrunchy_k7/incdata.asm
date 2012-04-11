; kkrunchy: include relevant binary data

bits    32
section .text

global  _depacker
global  _depackerSize

global  _depacker2
global  _depacker2Size

_depacker       incbin PACKNAME
_depackerSize   dd $-_depacker

_depacker2      incbin PACKNAME2
_depacker2Size  dd $-_depacker2