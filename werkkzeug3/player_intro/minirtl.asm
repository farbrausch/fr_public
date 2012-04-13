; This file is distributed under a BSD license. See LICENSE.txt for details.

; minimal runtime library

section .text

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

global _memset

_memset:
    push        edi
    mov         edi, [esp+8]
    mov         eax, [esp+12]
    mov         ecx, [esp+16]
    
    mov         ah, al
    movzx       edx, ax
    shl         eax, 16
    or          eax, edx
    
    push        ecx
    shr         ecx, 2
    jz          .tail
    
    rep         stosd
    
.tail:
    pop         ecx
    and         ecx, byte 3
    jz          .end
    
    rep         stosb
    
.end:
    pop         edi
    ret
    
global _memcpy

_memcpy:
    push        edi
    push        esi
    mov         edi, [esp+12]
    mov         esi, [esp+16]
    mov         ecx, [esp+20]
    
    push        ecx
    shr         ecx, 2
    jz          .tail
    
    rep         movsd
    
.tail:
    pop         ecx
    and         ecx, byte 3
    jz          .end
    
    rep         movsb
    
.end:
    pop         esi
    pop         edi
    ret
