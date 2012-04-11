; kkrunchy depacker (c callable)
; Fabian Giesen 2002.
; I hereby place this code in the public domain.

bits    32

%define ModelSizes 2+1+32+256+256+256

struc   dataArea
  .src    resd 1
  .code   resd 1
  .range  resd 1
  .move   resd 1
  .r0     resd 1
  .cmodel resd 2    ; code models
  .pmodel resd 1    ; prev match model
  .mmodel resd 32   ; match low models
  .lmodel resd 256  ; literal model
  .gmodel resd 256  ; gamma 0 model
  .hmodel resd 256  ; gamma 1 model
  .size:
endstruc

section .text

; ----------------------------------------------------------------------------

global    _CCADepackerA@8
_CCADepackerA@8:
          push      ebp
          push      esi
          push      edi
          push      ebx
          
          mov       esi, [esp + 16 + 8]
          sub       esp, dataArea.size
          mov       ebp, esp

          ; initialize pointers and code
          lodsd
          mov       dword [ebp + dataArea.src], esi
          bswap     eax
          mov       dword [ebp + dataArea.code], eax
          or        dword [ebp + dataArea.range], byte -1
          
          mov       dword [ebp + dataArea.move], 5
          mov       dword [ebp + dataArea.r0], 0

          ; initialize models
          lea       edi, [ebp + dataArea.cmodel]
          mov       ecx, ModelSizes
          xor       eax, eax
          mov       ah, 1024/256
          rep       stosd

          mov       edi, [esp + dataArea.size + 16 + 4]
          mov       esi, DecodeBit

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
.mtree    push      ebx
          lea       ebx, [ebx + edx * 4]
          call      esi
          pop       ebx
          adc       edx, edx
          lea       ecx, [eax + ecx * 2]
          test      dl, 16
          jz        .mtree
          
          lea       eax, [ecx + 1]
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
                    
.end      mov       eax, edi
          sub       eax, [esp + dataArea.size + 16 + 4]

          add       esp, dataArea.size
          pop       ebx
          pop       edi
          pop       esi
          pop       ebp
          
          ret       8

; ----------------------------------------------------------------------------

DecodeBit push      ecx
          mov       eax, [ebp + dataArea.range]         ; in ebx=model.
          shr       eax, 11
          imul      eax, [ebx]                          ; now eax=bound
          cmp       eax, [ebp + dataArea.code]
          mov       ecx, [ebp + dataArea.move]
          jbe       .one
          
          mov       [ebp + dataArea.range], eax
          mov       eax, 2048
          sub       eax, [ebx]
          shr       eax, cl
          add       [ebx], eax
          xor       eax, eax
          jmp       short .renorm

.one      sub       [ebp + dataArea.code], eax
          sub       [ebp + dataArea.range], eax
          mov       eax, [ebx]
          shr       eax, cl
          sub       [ebx], eax
          or        eax, byte -1

.renorm   test      byte [ebp + dataArea.range + 3], 0xff ; range<(1<<24)?
          jnz       .norenorm
          
          ; renormalize
          mov       ecx, [ebp + dataArea.src]
          inc       dword [ebp + dataArea.src]
          mov       cl, [ecx]
          shl       dword [ebp + dataArea.range], 8
          shl       dword [ebp + dataArea.code], 8
          mov       [ebp + dataArea.code], cl
          
.norenorm shr       eax, 31
          pop       ecx
          ret
          
          ; out: eax = bit; z-flag = bit is zero; c-flag = bit
          
; ----------------------------------------------------------------------------

DeGamma   push      eax
          xor       ecx, ecx                            ; in ebx=model
          inc       ecx
          mov       edx, ecx

.loop     push      ebx
          lea       ebx, [ebx + edx * 4]
          call      esi
          pop       ebx
          adc       dl, dl
          push      ebx
          lea       ebx, [ebx + edx * 4]
          call      esi
          pop       ebx
          adc       dl, dl
          lea       ecx, [eax + ecx * 2]
          test      dl, 2
          jnz       .loop
          pop       eax
          
          ret
          
          ; out ecx=value; edx destroyed.

; ----------------------------------------------------------------------------
