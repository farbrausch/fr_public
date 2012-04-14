bits 32
section .data

global _theTune
_theTune:
  incbin "../pzero_new.v2m"

global _theTune2
_theTune2:
  incbin "../v2_zeitmaschine_new.v2m"
  
;global _theSoundbank
;_theSoundbank:
;  incbin "../presets.v2b"
  
%ifdef NDEBUG
global __fltused
__fltused dd 0
%endif
