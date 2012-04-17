; loader tune (yeezzz!)

bits      32
section   .text

global    _loaderTune
_loaderTune   incbin "tpinv.v2m"

global    _loaderTuneEnd
_loaderTuneEnd: