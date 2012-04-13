; This file is distributed under a BSD license. See LICENSE.txt for details.

; dispack decoder
; ryg/farbrausch 2003

bits 32

%define NBUFFERS 22
%define BUFFER dataArea.buffer

struc dataArea
.buffer     resd NBUFFERS
.offset     resd 1
.lastJump   resd 1
.nextFunc   resd 1
.macroPtr   resd 1
.funcTable  resd 256
.instTable  resd 255
.size:
endstruc

; ----------------------------------------------------------------------------

section .data

%define fNM   0x0
%define fAM   0x1
%define fMR   0x2
%define fMO   0x3
%define fMODE 0x3

%define fNI   0x0
%define fBI   0x4
%define fDI   0x8
%define fWI   0xc

%define fAD   0x0
%define fBR   0x4
%define fDR   0xc

%define fERR  0x0

; ----------------------------------------------------------------------------

DisUnfilter:
        mov           edi, 'DCOD'
        
        mov           ebx, edi
        sub           ebx, 4
        mov           [ebp+dataArea.offset], ebx
        
        xor           eax, eax
        mov           [ebp+dataArea.lastJump], eax
        mov           [ebp+dataArea.macroPtr], eax
        inc           eax
        mov           [ebp+dataArea.nextFunc], eax
        mov           [ebp+dataArea.funcTable], eax
        
        lea           ebx, [esi+NBUFFERS*4]
        xor           ecx, ecx
        
.init   lodsd
        mov           [ebp+BUFFER+ecx*4], ebx
        add           ebx, eax
        inc           ecx
        cmp           cl, NBUFFERS
        jne           .init
        
        mov           esi, [ebp+BUFFER+1*4]
        xchg          esi, [ebp+BUFFER]
        
        ; read opcode table
        push          edi
        lea           edi, [ebp+dataArea.instTable]
        lodsb
        movzx         ecx, al
        mov           ebx, Tables + ('R'<<24) + ('P'<<16)
        
.rdinst mov           eax, esi
        stosd
        
        xor           eax, eax
        lodsb
        cmp           al, 0x66
        jne           .rdnos
        lodsb

.rdnos  cmp           al, 0x0f
        jne           .rdntwo
        
        lodsb
        mov           ah, 1

.rdntwo shr           eax, 1
        xlatb
        jnc           .rdfok
        shr           al, 4
.rdfok  test          al, fMR
        jz            .rdnmrm
        inc           esi

.rdnmrm loop          .rdinst
        pop           edi
        
.main   xor           eax, eax
        cmp           [ebp+dataArea.macroPtr], eax
        je            .nomacr
        xchg          eax, [ebp+dataArea.macroPtr]
        xchg          esi, eax

.nomacr cmp           esi, [ebp+BUFFER]
        db            0x0f, 0x84 ; je near
        db            'ETRY'
        
        ; get macro instruction
        lodsb
        cmp           al, 0xff ; no macro instruction?
        je            .gotins
        
        mov           [ebp+dataArea.macroPtr], esi
        movzx         eax, al
        mov           esi, [ebp+dataArea.instTable+eax*4]

.gotins xor           eax, eax
        cmp           [ebp+dataArea.nextFunc], eax
        lodsb
        je            .testes
        
        cmp           al, 0xcc ; int3 (padding used by vc)
        je            .testes

        lea           ecx, [edi-4]
        sub           ecx, [ebp+dataArea.offset]
        
        mov           ebx, [ebp+dataArea.funcTable]
        mov           [ebp+dataArea.funcTable+ebx*4], ecx
.incm   inc           byte [ebp+dataArea.funcTable]
        jz            .incm
        
        mov           byte [ebp+dataArea.nextFunc], 0
        
.testes cmp           al, 0xce ; escape
        jne           .noesc
        
        movsb
        jmp           short .main
        
.noesc  stosb

        xor           edx, edx
        cmp           al, 0x66 ; operand size prefix
        jne           .nopfx

        mov           dh, 1        
        lodsb
        stosb
        
.nopfx  mov           bl, al
        cmp           bl, 0xcc
        je            .isret
        sub           bl, 0xc2
        cmp           bl, 1
        ja            .noret
        
.isret  mov           byte [ebp+dataArea.nextFunc], 1 ; inc?
        
.noret  mov           ebx, Tables + ('R'<<24) + ('P'<<16)
        cmp           al, 0x0f ; two-byte opcode
        jne           .notwo
        
        lodsb
        stosb
        mov           ah, 1
        
.notwo  shr           eax, 1                      ; get flags
        xlatb
        jnc           .flagok
        shr           al, 4
.flagok and           al, 0xf
        mov           cl, al
        
        test          cl, fMR                     ; mod/rm follows?
        jz            near .nomdrm
        
        lodsb
        stosb
        mov           ch, al
        
        mov           al, cl
        and           al, fMODE
        cmp           al, fMO
        jne           .noxtra
        
        mov           cl, fMR|fNI
        test          ch, 0x38
        jnz           .noxtra
        mov           bl, [edi-2]
        test          bl, 0x08
        jnz           .noxtra
        add           cl, fBI
        
.noxtra and           ch, 0xc7
        cmp           ch, 0xc4
        je            .nosib
        mov           al, ch
        and           al, 0x07
        cmp           al, 0x04
        jne           .nosib
        
        xchg          esi, [ebp+BUFFER+21*4]
        movsb
        xchg          esi, [ebp+BUFFER+21*4]
        
.nosib  mov           dl, ch
        and           dl, 0xc0
        cmp           dl, 0x40
        jne           .nodis8
        
        movzx         ebx, ch
        and           bl, 0x07
        
        xchg          esi, [ebp+BUFFER+1*4+ebx*4]
        movsb
        xchg          esi, [ebp+BUFFER+1*4+ebx*4]
        
.nodis8 cmp           dl, 0x80
        je            .dis32
        cmp           ch, 0x05
        je            .dis32
        test          dl, dl
        jnz           .nomdrm
        mov           al, [edi-1]
        and           al, 0x07
        cmp           al, 0x05
        jne           .nomdrm
        
.dis32  xor           ebx, ebx
        cmp           ch, 5
        jne           .nomr5
        inc           ebx
.nomr5  xchg          esi, [ebp+BUFFER+13*4+ebx*4]
        lodsd
        dec           esi
        xchg          esi, [ebp+BUFFER+13*4+ebx*4]
        xchg          esi, [ebp+BUFFER+18*4]
        shl           eax, 8
        lodsb
        xchg          esi, [ebp+BUFFER+18*4]
        ror           eax, 8
        stosd

.nomdrm mov           al, cl
        and           al, fMODE
        cmp           al, fAM
        jne           near .noaddr
        
        shr           cl, 2
        jnz           .noad
        
        xchg          esi, [ebp+BUFFER+15*4]
        lodsd
        dec           esi
        xchg          esi, [ebp+BUFFER+15*4]
        xchg          esi, [ebp+BUFFER+19*4]
        shl           eax, 8
        lodsb
        xchg          esi, [ebp+BUFFER+19*4]
        ror           eax, 8
        stosd
        jmp           .main
        
.noad   dec           cl
        jnz           .dwdrl
        
        xchg          esi, [ebp+BUFFER+9*4]
        movsb
        xchg          esi, [ebp+BUFFER+9*4]        
        jmp           .main
        
.dwdrl  xor           ebx, ebx
        cmp           byte [edi-1], 0xe8
        je            .dwcal
        
        xchg          esi, [ebp+BUFFER+20*4]
        lodsw
        xchg          esi, [ebp+BUFFER+20*4]
        xchg          esi, [ebp+BUFFER+17*4]
        shl           eax, 16
        lodsw
        xchg          esi, [ebp+BUFFER+17*4]
        
        shr           eax, 1
        jnc           .jmpnc
        not           eax
        
.jmpnc  add           eax, [ebp+dataArea.lastJump]
        mov           [ebp+dataArea.lastJump], eax
        jmp           short .storad
        
.dwcal  xor           eax, eax
        xchg          esi, [ebp+BUFFER+16*4]
        lodsb
        xchg          esi, [ebp+BUFFER+16*4]
        test          al, al
        jz            .dcesc
        
        mov           eax, [ebp+dataArea.funcTable+eax*4]
        jmp           short .storad
        
.dcesc  xchg          esi, [ebp+BUFFER+20*4]
        lodsw
        xchg          esi, [ebp+BUFFER+20*4]
        xchg          esi, [ebp+BUFFER+16*4]
        shl           eax, 16
        lodsw
        xchg          esi, [ebp+BUFFER+16*4]
        
        mov           ebx, [ebp+dataArea.funcTable]
        mov           [ebp+dataArea.funcTable+ebx*4], eax
.dcinc  inc           byte [ebp+dataArea.funcTable]
        jz            .dcinc

.storad sub           eax, edi
        add           eax, [ebp+dataArea.offset]
        stosd
        jmp           .main
        
.noaddr shr           cl, 2
        jz            near .main
        
        dec           cl
        jnz           .dwow
        
        xchg          esi, [ebp+BUFFER+10*4]
        movsb
        xchg          esi, [ebp+BUFFER+10*4]
        jmp           .main
        
.dwow   dec           cl
        jnz           .word
        test          dh, dh
        jnz           .word
        
        xchg          esi, [ebp+BUFFER+12*4]
        movsd
        xchg          esi, [ebp+BUFFER+12*4]
        jmp           .main
        
.word   xchg          esi, [ebp+BUFFER+11*4]
        movsw
        xchg          esi, [ebp+BUFFER+11*4]
        jmp           .main
        
Tables:
  db "DEPACKTABLE"
  times (256-($-Tables)) db 0
  ; the third table is implemented as code
        
