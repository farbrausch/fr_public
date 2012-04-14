
global _presetbank
global _pbsize  

section .data

_presetbank:
  incbin "../presets.v2b"
  
pbend:
_pbsize: dd pbend-_presetbank
  
 