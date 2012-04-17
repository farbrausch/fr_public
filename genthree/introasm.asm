; This file is distributed under a BSD license. See LICENSE.txt for details.
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
     
	section .text
         
	global	_memcpy
_memcpy:
    push	esi
    push	edi
    lea		ecx, [esp+12]
    mov		edi, [ecx]
    mov		eax, edi
    mov		esi, [ecx+4]
    mov		ecx, [ecx+8]
    rep		movsb
    pop		edi
    pop		esi
	ret

	global _memset
_memset:
	push	edi
	lea		ecx, [esp+8]
	mov		edi, [ecx]
	push	edi
	mov		al, [ecx+4]
	mov		ecx, [ecx+8]
	rep		stosb
	pop		eax
	pop		edi
	ret

global	__ftol2
global  __ftol2_sse

__ftol2:
__ftol2_sse:
		push				ebp
		mov					ebp, esp
		sub					esp, 20h
		and					esp, -16
		fld					st0
		fst					dword [esp+18h]
		fistp				qword [esp+10h]
		fild				qword [esp+10h]
		mov					edx, dword [esp+18h]
		mov					eax, dword [esp+10h]
		test				eax, eax
		je					.int_qnan_or_zero

.not_int_qnan:		
		fsubp				st1, st0
		test				edx, edx
		jns					.positive
		fstp				dword [esp]
		mov					ecx, [esp]
		xor					ecx, 80000000h
		add					ecx, 7fffffffh
		adc					eax, 0
		mov					edx, dword [esp+14h]
		adc					edx, 0
		jmp					short .exit
		
.positive:
		fstp				dword [esp]
		mov					ecx, dword [esp]
		add					ecx, 7fffffffh
		sbb					eax, 0
		mov					edx, dword [esp+14h]
		sbb					edx, 0
		jmp					short .exit
		
.int_qnan_or_zero:
		mov					edx, dword [esp+14h]
		test				edx, 7fffffffh
		jne					.not_int_qnan
		fstp				dword [esp+18h]
		fstp				dword [esp+18h]
		
.exit:
		leave
		ret  
