;*************************************************************************************
;*************************************************************************************
;**                                                                                 **
;**  Tinyplayer - LibV2 example                                                     **
;**  written by Tammo 'kb' Hinrichs 2000-2008                                       **
;**  This file is in the public domain                                              **
;**  "Patient Zero" is (C) Melwyn+LB 2005, do not redistribute                      **
;**                                                                                 **
;**  Compile with NASM 0.97 or YASM                                                 **
;**                                                                                 **
;*************************************************************************************
;*************************************************************************************

bits 32
section .data

global _theTune
_theTune:
  incbin "..\v2m\pzero_new.v2m"

;*************************************************************************************
;**                                                                                 **
;** Some replacements for things VC2005 needs when compiling without CRT            **
;**                                                                                 **
;*************************************************************************************

%ifdef NDEBUG
global __fltused
__fltused dd 0

section .text

global __chkstk
__chkstk ret

%endif
