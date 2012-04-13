; This file is distributed under a BSD license. See LICENSE.txt for details.

; kkrunchy depacker

bits    32

%define ModelSizes 2+1+32+256+256+256

struc   dataArea
  .src    resd 1
  .csub   resd 1
  .range  resd 1
  .move   resd 1
  .r0     resd 1
  .cmodel resd 2    ; code models
  .pmodel resd 1    ; prev match model
  .mmodel resd 32   ; match low models
  .lmodel resd 256  ; literal model
  .gmodel resd 256  ; gamma 0 model
  .hmodel resd 256  ; gamma 1 model
  .rest   resd 1024-ModelSizes ; padding
  .size:
endstruc

section .text

; ----------------------------------------------------------------------------

DePacker  mov       ebp, 'WORK'
          
          ; initialize pointers and state
          mov       dword [ebp + dataArea.src], 'ESI0'
          dec       dword [ebp + dataArea.range]
          mov       byte [ebp + dataArea.move], 5

          ; initialize models
          lea       edi, [ebp + dataArea.cmodel]
          xor       eax, eax
          mov       ah, 1024/256
          mov       ecx, eax
          rep       stosd
          
          mov       edi, 'EDI0'
          push      edi
          mov       esi, DecodeBit + ('R' << 24) + ('P' << 16)

.literal  xor       ecx, ecx
          inc       ecx
          dec       dword [ebp + dataArea.move]
          
.litbit   lea       ebx, [ebp + dataArea.lmodel + ecx * 4]
          call      esi
          adc       cl, cl
          jnc       .litbit
          
          inc       dword [ebp + dataArea.move]
          xchg      eax, ecx
          stosb
          or        ecx, byte -1

.getcode  lea       ebx, [ebp + dataArea.cmodel + 4 + ecx * 4]
          call      esi
          jz        .literal
          
          ; match
          jecxz     .noprev
          
          lea       ebx, [ebp + dataArea.pmodel]
          call      esi
          jz        .noprev
          
          lea       ebx, [ebp + dataArea.hmodel]
          call      DeGamma
          mov       eax, [ebp + dataArea.r0]
          jmp       short .copy
          
.noprev   lea       ebx, [ebp + dataArea.gmodel]
          call      DeGamma
          dec       ecx
          dec       ecx
          js        .end
          lea       ebx, [ebp + dataArea.mmodel]
          jz        .nonext
          add       ebx, byte 16*4

.nonext   xor       edx, edx
          inc       edx

.mtree    call      DeBitTree
          lea       ecx, [eax + ecx * 2]
          test      dl, 16
          jz        .mtree
          
          inc       ecx
          xchg      eax, ecx
          
          lea       ebx, [ebp + dataArea.hmodel]
          call      DeGamma
          
          cmp       eax, 2048
          sbb       ecx, byte -1
          cmp       eax, byte 96
          sbb       ecx, byte -1

.copy     mov       [ebp + dataArea.r0], eax
          push      esi
          mov       esi, edi
          sub       esi, eax
          rep       movsb
          pop       esi
          jmp       short .getcode

          ; imports
.end      mov       esi, 'IMPO'
          mov       ebx, 'LLIB'
          push      ebp
          
.iloop    inc       esi
          lodsd
          test      eax, eax
          jz        .iend
          xchg      eax, edi
          push      esi
          call      [ebx]
          test      eax, eax
          jz        .ierr
          xchg      eax, ebp

.lfunc    lodsb
          test      al, al
          jnz       .lfunc
          cmp       [esi], al
          je        .iloop
          js        .iord
          push      esi
.iget     push      ebp
          call      [ebx+4]
          stosd
          test      eax, eax
          jnz       .lfunc

.ierr     inc       eax
          pop       ebx
          pop       ecx
          ret

.iord     inc       esi
          xor       eax, eax
          lodsw
          push      eax
          jmp       short .iget

.iend     pop       ebp
          ret

; ----------------------------------------------------------------------------

DecodeBit push      ecx                                 ; in ebx=model.
          mov       eax, [ebp + dataArea.range]
          shr       eax, 11
          mov       ecx, [ebp + dataArea.src]
          imul      eax, [ebx]                          ; now eax=bound
          mov       ecx, [ecx]
          bswap     ecx
          sub       ecx, [ebp + dataArea.csub]
          cmp       eax, ecx
          mov       ecx, [ebp + dataArea.move]
          jbe       .one
          
          mov       [ebp + dataArea.range], eax
          xor       eax, eax
          mov       ah, 2048/256
          sub       eax, [ebx]
          shr       eax, cl
          add       [ebx], eax
          xor       eax, eax
          jmp       short .renorm

.one      add       [ebp + dataArea.csub], eax
          sub       [ebp + dataArea.range], eax
          mov       eax, [ebx]
          shr       eax, cl
          sub       [ebx], eax
          or        eax, byte -1

.renorm   test      byte [ebp + dataArea.range + 3], 0xff ; range<(1<<24)?
          jnz       .norenorm
          
          ; renormalize
          inc       dword [ebp + dataArea.src]
          shl       dword [ebp + dataArea.range], 8
          shl       dword [ebp + dataArea.csub], 8
          
.norenorm shr       eax, 31
          pop       ecx
          ret
          
          ; out: eax = bit; z-flag = bit is zero; c-flag = bit

; ----------------------------------------------------------------------------

DeBitTree push      ebx
          lea       ebx, [ebx + edx * 4]
          call      esi
          pop       ebx
          adc       dl, dl
          ret
          
; ----------------------------------------------------------------------------

DeGamma   push      eax                                 ; in ebx=model
          xor       ecx, ecx
          inc       ecx
          mov       edx, ecx

.loop     call      DeBitTree
          call      DeBitTree
          lea       ecx, [eax + ecx * 2]
          test      dl, 2
          jnz       .loop
          pop       eax
          
          ret

          ; out ecx=value; edx destroyed.

; ----------------------------------------------------------------------------
