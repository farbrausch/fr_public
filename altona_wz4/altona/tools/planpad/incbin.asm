;/*+**************************************************************************/
;/***                                                                      ***/
;/***   This file is distributed under a BSD license.                      ***/
;/***   See LICENSE.txt for details.                                       ***/
;/***                                                                      ***/
;/**************************************************************************+*/

; ++++

			section .text
			bits    32

			global	WireTXT
			global	_WireTXT

WireTXT:
_WireTXT:
      incbin  "planpad.wire.txt"
			dw		0
			