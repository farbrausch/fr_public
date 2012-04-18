; kkrunch_p7 depacker (asm version)
; fabian "ryg/farbrausch" giesen 2006
; I hereby place this code in the public domain.

; ---- uninitialized data

bits          32

section       .bss

%define MEMSHIFT  23
%define MEM       (1<<MEMSHIFT)
%define NMODEL    11
%define NINPUT    48
%define NWEIGHT   (256+256+16+128)
%define MAXLEN    2047
%define USEAPM    1
%define APMSIZE   8192

struc ContextModel
  .cpr        resd 1
  .cps        resd 1
  .ctx        resd 1
  .st         resd 1
  .size:
endstruc

struc Work
  .src        resd 1
  .dst        resd 1
  .csub       resd 1
  .range      resd 1
  .outsize    resd 1
  .c0         resd 1
  .c0s        resd 1
  .bpos       resd 1
  .lentemp    resd 1
  .matchp     resd 1
  .matchl     resd 1
  .matchw     resd 1
  .pr         resd 4
%if USEAPM
  .outprob    resd 1 ; MUST follow pr
%endif  
  .ctx        resd 3
  .bit        resd 1
  .bitscaled  resd 1
  .origdst    resd 1
  .zeroprob   resd 1
  .bitcounter resd 1
%if USEAPM  
  .APMi       resd 1
%endif  
  .cm         resb ContextModel.size * NMODEL
  .match      resd MEM/16
  .modelMem   resb MEM
  .tx         resw NINPUT
  .wx         resw NINPUT*NWEIGHT
  .tx2        resw 4
  .wx2        resw 8
  .runTable   resd 256
  .stateCode  resb 256*2  ; must follow immediately after runTable
  .stateNext  resb 256*2
  .stateMap   resd 256
  .stretch    resd 4096
%if USEAPM
  .APM        resd APMSIZE*33
%endif  
  .size:
endstruc

WorkData      resb Work.size

; ---- fixups

extern        PACKED
extern        DEPACKED
extern        DEPACKEDSIZE
extern        IMPORTS
extern        LOADLIBRARY

extern        PATCHOFFS
extern        PATCHVALUE

global        STAGE0ENTRY

; ---- code

section       .text

; squash
;   in: eax
;   out: eax
;   destroy: ecx,edx

squash:
  cmp       eax, -2047
  jge       .notsmall
  xor       eax, eax
  ret
  
.notsmall:
  cmp       eax, 2047
  jle       .notbig
  mov       eax, 4095
  ret
  
.notbig:
  mov       ecx, eax
  sar       ecx, 7
  lea       ecx, [squashTab+ecx*2+16*2]
  and       eax, byte 127
  mov       edx, [ecx+2]
  sub       edx, [ecx]
  movzx     edx, dx
  imul      eax, edx
  add       eax, byte 64
  sar       eax, 7
  add       ax, [ecx]
  ret
  
; contextHash
;   in: esi=model
;       eax=i
;   out: eax=p
;   destroy: edx

contextHash:
  mov       edx, eax
  shr       edx, 24
  and       eax, ((MEM / 4) - 1) & ~1
  lea       eax, [ebp+Work.modelMem+eax*4]
  
  cmp       dl, [eax]
  jne       .notequal1
  inc       eax
  ret
  
.notequal1:
  add       eax, byte 4
  cmp       dl, [eax]
  jne       .notequal2
  inc       eax
  ret
  
.notequal2:    
  mov       dh, [eax+1]
  cmp       dh, [eax-3]
  jbe       .noswap
  sub       eax, byte 4
  
.noswap:
  movzx     edx, dl
  mov       [eax], edx
  inc       eax
  ret
  
; train
;   in: eax=err
;       esi=t
;       edi=w
;       ecx=n/4
;   out: esi,edi advanced.
train:
  movd      mm0, eax
  punpcklwd mm0, mm0
  punpcklwd mm0, mm0
  pcmpeqb   mm1, mm1
  psrlw     mm1, 15

.lp:
  movq      mm3, [esi]
  movq      mm2, [edi]
  paddsw    mm3, mm3
  pmulhw    mm3, mm0
  paddsw    mm3, mm1
  psraw     mm3, 1
  paddsw    mm2, mm3
  movq      [edi], mm2
  add       esi, byte 8
  add       edi, byte 8
  loop      .lp  
  
  ret

; decodebit
;   in: eax=prob
;   out: edx=bit; eax, ecx destroyed

decodebit:
  mov       ecx, [ebp+Work.src]
  mul       dword [ebp+Work.range]
  shrd      eax, edx, 12
  
  mov       ecx, [ecx]
  bswap     ecx
  sub       ecx, [ebp+Work.csub]
  xor       edx, edx
  cmp       eax, ecx
  ja        .one
  
  add       [ebp+Work.csub], eax
  sub       [ebp+Work.range], eax
  jmp       short .renorm

.one:
  mov       [ebp+Work.range], eax
  inc       edx

.renorm:
  cmp       byte [ebp+Work.range+3], 0
  jne       .done
  
  ; renormalize
  inc       dword [ebp+Work.src]
  shl       dword [ebp+Work.range], 8
  shl       dword [ebp+Work.csub], 8
  jmp       short .renorm
  
.done:
  ret

; ---- the actual depacker entry

STAGE0ENTRY:
  mov       ebp, WorkData
  mov				dword [ebp+Work.src], PACKED
  mov       eax, DEPACKED
  mov				dword [ebp+Work.dst], eax
  mov       dword [ebp+Work.origdst], eax
  push      eax
  mov				dword [ebp+Work.outsize], DEPACKEDSIZE
    
; ---- model initialization (assumes everything filled with zero)

  dec       dword [ebp+Work.range]  
  inc       dword [ebp+Work.c0]
  inc       dword [ebp+Work.zeroprob]
  mov       byte [ebp+Work.bpos], 8
  mov       eax, 2048
  lea       edi, [ebp+Work.pr]
  stosd
  stosd
  stosd
  stosd
%if USEAPM  
  stosd
%endif
  
  ; initialize run table via numerical integration of 1/x
  mov       ebx, 14155776
  mov       edi, WorkData+Work.runTable+4

  xor       ecx, ecx
  inc       ecx
  
.runTableLoop:
  lea       esi, [ecx*2+1]
  mov       eax, 774541002
  cdq
  div       esi
  add       ebx, eax
  mov       eax, ebx
  shr       eax, 21
  stosd
  inc       cl
  jnz       .runTableLoop
  
  ; build state table.
  ; note: edi already points to stateCode
  ; assumes ecx=0
  mov       esi, WorkData+Work.stateNext
  xor       ebx, ebx
  mov       cl, 4
  
.stateTableLoop:
  xor       ebx, byte 1
  mov       ah, [edi+ebx]
  cmp       ah, 2
  jbe       .log_underflow
  
  movzx     eax, ah
  mov       eax, [WorkData+Work.runTable+eax*4-4]
  shl       eax, 2
  dec       ah
  
.log_underflow:
  xor       ebx, byte 1
  mov       al, [edi+ebx]
  
  inc       eax
  cmp       al, 40
  jbe       .no_inc_overflow
  mov       al, 40
  
.no_inc_overflow:
  test      bl, 1
  jz        .bit_even
  xchg      al, ah
  
.bit_even:
  push      edi
  mov       edx, ecx
  repne     scasw
  je        .found
  inc       edx
  scasw
  
.found:
  not       ecx
  add       ecx, edx
  mov       [edi-2], ax
  mov       [esi], cl
  inc       esi
  mov       ecx, edx
  pop       edi
  inc       ebx
  
  cmp       bh, 2
  jne       .stateTableLoop
    
  ; build initial state map.
  ; note that ecx=0x100 at this point
  mov       esi, edi
  mov       edi, WorkData+Work.stateMap
  
.stateMapLoop:
  xor       eax, eax
  lodsb
  mov       ebx, eax
  lodsb
  inc       eax
  inc       ebx
  add       ebx, eax
  shl       eax, 16
  cdq
  div       ebx
  stosd
  loop      .stateMapLoop
  
  ; transform state code table to and masks
  mov       ch, 2
  mov       esi, WorkData+Work.stateCode
  mov       edi, esi
  
.stateCodeLoop:
  lodsb
  sub       al, 1
  salc
  stosb
  loop      .stateCodeLoop
  
  ; build stretch table (inverse of squash)
  mov       edi, WorkData+Work.stretch
  or        ebx, byte -1 ; =previous i
  mov       eax, -2047
  
.stretchLoop:
  push      eax
  call      squash

  mov       ecx, eax
  sub       ecx, ebx
  mov       ebx, eax
  pop       eax
  rep       stosd
  
  inc       eax
  cmp       eax, 2048
  jle       .stretchLoop
  
  ; initialize context model
  ; assumes ecx=0 here
  lea       esi, [ebp+Work.cm]
  mov       cl, NMODEL
  
.ctxModelLoop:
  xor       eax, eax
  inc       eax
  call      contextHash
  mov       [esi+ContextModel.cpr], eax
  xor       eax, eax
  call      contextHash
  mov       [esi+ContextModel.cps], eax
  add       esi, byte ContextModel.size
  loop      .ctxModelLoop

%if USEAPM
  ; initialize APM
  lea       edi, [ebp+Work.APM]
  mov       esi, edi
  push      byte -16
  pop       eax
  
.APMLoop:
  push      eax
  shl       eax, 7
  call      squash
  shl       eax, 4
  stosd
  pop       eax
  inc       eax
  cmp       eax, byte 16
  jle       .APMLoop

  ; and clone it!
  mov       ecx, (APMSIZE-1)*33
  rep       movsd
%endif  

; ---- main decoding loop

  xor       ebx, ebx ; =byte
  inc       ebx
  
.decodebitloop:
  xor       eax, eax
  cmp       eax, [ebp+Work.bitcounter]
  jne       .nozeropage
  
  mov       eax, [ebp+Work.zeroprob]
  call      decodebit
  
  mov       eax, [ebp+Work.zeroprob]
  neg       edx
  inc       eax
  shr       edx, 32-12 ; eqv. to and edx, 4095 here
  add       eax, edx
  shr       eax, 1
  mov       [ebp+Work.zeroprob], eax
  
  test      edx, edx
  jz        .nozeropage
  
  mov       eax, 8192
  add       dword [ebp+Work.dst], eax
  sub       dword [ebp+Work.outsize], eax
  jmp       short .decodebitloop

.nozeropage:
%if USEAPM
  mov       eax, [ebp+Work.outprob]
%else
  mov       eax, [ebp+Work.pr+12]
%endif  
  cmp       ah, 8
  adc       eax, byte 0
  
  call      decodebit

  ; byte writing+prepare for model
  mov       [ebp+Work.bit], edx
  inc       word [ebp+Work.bitcounter]
  shr       edx, 1
  adc       bl, bl
  jnc       .decodenobytedone

  mov       eax, [ebp+Work.dst]
  mov       [eax], bl
  inc       dword [ebp+Work.dst]
  mov       bl, 1

  dec       dword [ebp+Work.outsize]
  jnz       .decodenobytedone

  ; ---- import processing
.decodedone:
  emms
  pop       eax
  push      dword PATCHOFFS
  push      dword PATCHVALUE
  push      eax
  mov       esi, IMPORTS
  mov       ebx, LOADLIBRARY
%if 0
  push      ebp
  
.imploop:
  inc       esi
  lodsd
  test      eax, eax
  jz        .impend
  xchg      eax, edi
  push      esi
  call      [ebx]
  test      eax, eax
  jz        .imperr
  xchg      eax, ebp
  
.impfunc:
  lodsb
  test      al, al
  jnz       .impfunc
  cmp       [esi], al
  je        .imploop
  js        .impord
  push      esi
  
.impget:
  push      ebp
  call      [ebx+4]
  stosd
  test      eax, eax
  jnz       .impfunc
  
.imperr:
  inc       eax
  pop       ebx
  pop       ebx
  ret
  
.impord:
  inc       esi
  xor       eax, eax
  lodsw
  push      eax
  jmp       short .impget        

.impend:
  pop       ebp
%endif  
  ret

.decodenobytedone:
  ; ---- model main loop
  push      ebx

  mov       ecx, [ebp+Work.bit]
  shl       ecx, 16
  sub       ecx, byte -128
  mov       [ebp+Work.bitscaled], ecx
  
  ; update mixers
  xor       edx, edx
  
.updmixer:
  mov       eax, [ebp+Work.bit]
  shl       eax, 12
  sub       eax, [ebp+Work.pr+edx*4]
  imul      eax, byte 7
  
  cmp       dl, 3
  je        .updmixerdone
  
  mov       edi, [ebp+Work.ctx+edx*4]
  imul      edi, byte (NINPUT*2)
  lea       edi, [ebp+Work.wx+edi]
  mov       esi, WorkData+Work.tx
  push      byte (NINPUT/4)
  pop       ecx
  call      train
  
  inc       edx
  jmp       short .updmixer
  
.updmixerdone:
  ; assumes ecx=0
  mov       edi, WorkData+Work.wx2
  mov       esi, WorkData+Work.tx2
  inc       ecx
  call      train
  
  ; update c0 and position in bit
  mov       eax, [ebp+Work.bit]
  shl       dword [ebp+Work.c0], 1
  add       [ebp+Work.c0], eax
  
  dec       dword [ebp+Work.bpos]
  jnz       near .nonewbyte
  
  ; assumes ecx=0
  mov       byte [ebp+Work.bpos], 8
  inc       ecx
  mov       [ebp+Work.c0], ecx
  
  ; update context models
  lea       esi, [ebp+Work.cm]
  xor       eax, eax
  push      byte NMODEL
  pop       ecx
  
.updcontext:
  and       eax, byte -2
  mov       [esi+ContextModel.ctx], eax
  
  ; update run context
  mov       ebx, [esi+ContextModel.cpr]
  mov       edi, [ebp+Work.dst]
  mov       dh, [edi-1] ; bufPtr[-1]
  cmp       dh, [ebx+1] ; cpr[1]
  je        .norunflush
  mov       dl, 0
  mov       [ebx], dx
  
.norunflush:
  inc       byte [ebx]
  inc       eax
  call      contextHash
  mov       [esi+ContextModel.cpr], eax
  
  ; fnv hash function
  ; assumes edi=_bufPtr
  mov       eax, 0x811c9dc5
  inc       ecx
  imul      eax, ecx
  dec       ecx
  mov       bl, [masks+ecx-1]
  mov       dh, [bitm+ecx-1]
  
.hashnext:
  dec       edi
  shr       bl, 1
  jnc       .hashnotset
  
  mov       dl, [edi]
  and       dl, dh
  xor       al, dl
  imul      eax, 0x01000193
  jmp       short .hashnext

.hashnotset:
  jnz       .hashnext
  add       esi, ContextModel.size
  loop      .updcontext
  
  ; match processing
  ; assumes esi points to "match"
  and       eax, (MEM/16)-1
  mov       edi, [ebp+Work.dst]
  xchg      edi, [esi+eax*4] ; now edi=Match[h]
  mov       esi, [ebp+Work.dst]
  
  mov       ecx, [ebp+Work.matchl]
  jecxz     .searchmatch
  
  inc       ecx
  inc       dword [ebp+Work.matchp]
  jmp       short .updmatch
  
.searchmatch:
  test      edi, edi
  jz        .updmatch
  
  mov       [ebp+Work.matchp], edi

  std
  cmpsb

  mov       ecx, edi
  sub       ecx, [ebp+Work.origdst]
  mov       [ebp+Work.lentemp], ecx
  repe      cmpsb
  cld
  
  je        .allequal
  inc       ecx
  
.allequal:
  neg       ecx
  add       ecx, [ebp+Work.lentemp]

.updmatch:
  cmp       ecx, MAXLEN
  jbe       .noverylongmatch
  mov       ecx, MAXLEN
  
.noverylongmatch:
  mov       [ebp+Work.matchl], ecx
  
  cmp       ecx, byte 32
  jbe       .nolongmatch
  push      byte 32
  pop       ecx
  
.nolongmatch:
  shl       ecx, 6
  mov       [ebp+Work.matchw], ecx

.nonewbyte:
  ; start model mixing
  mov       ecx, [ebp+Work.c0]
  shl       ecx, 3
  mov       [ebp+Work.c0s], ecx
  
  ; constant model
  mov       edi, WorkData+Work.tx
  push      byte 127
  pop       eax
  stosw
  
  ; match model
  xor       eax, eax
  cmp       eax, [ebp+Work.matchl]
  je        .nomatch

  mov       ebx, [ebp+Work.matchp]
  movzx     ebx, byte [ebx]
  inc       bh
  mov       cl, [ebp+Work.bpos]
  shr       ebx, cl
  sbb       edx, edx
  cmp       ebx, [ebp+Work.c0]
  jne       .nomatch
  
  mov       eax, [ebp+Work.matchw]
  xor       eax, edx
  sub       eax, edx
  jmp       short .matchdone
  
.nomatch:
  mov       [ebp+Work.matchl], eax  
  
.matchdone:  
  stosw
  
  mov       eax, [ebp+Work.matchl]
  cmp       eax, 400
  jbe       .contextmodels
  
  mov       dword [ebp+Work.ctx], 512+14
  jmp       .mix

  ; context models
.contextmodels:
  lea       esi, [ebp+Work.cm]
  xor       eax, eax
  mov       ah, 2
  mov       [ebp+Work.ctx+8], eax
  push      byte NMODEL
  pop       ecx
  
.contextloop:
  ; run model
  push      ecx
  
  mov       eax, [esi+ContextModel.cpr]
  movzx     edx, byte [eax+1]
  movzx     eax, byte [eax]
  mov       cl, [ebp+Work.bpos]
  inc       dh
  mov       eax, [WorkData+Work.runTable+eax*4]
  shr       edx, cl
  sbb       ecx, ecx
  xor       eax, ecx
  sub       eax, ecx
  
  cmp       edx, [ebp+Work.c0]
  je        .run
  xor       eax, eax
  
.run:
  stosw
  pop       ecx
  
  ; nonstationary context
  mov       eax, [esi+ContextModel.cps]
  mov       edx, [ebp+Work.bit]
  movzx     ebx, byte [eax]
  add       ebx, ebx
  add       ebx, edx
  mov       bl, [WorkData+Work.stateNext+ebx]
  mov       [eax], bl

  mov       eax, [esi+ContextModel.ctx]
  xor       eax, [ebp+Work.c0s]
  call      contextHash
  
.nsset:
  mov       [esi+ContextModel.cps], eax
  
  ; state mapping
  mov       ebx, [esi+ContextModel.st]
  mov       eax, [ebp+Work.bitscaled]
  lea       ebx, [WorkData+Work.stateMap+ebx*4]
  sub       eax, [ebx]
  sar       eax, 8
  add       [ebx], eax
  mov       eax, [esi+ContextModel.cps]
  movzx     ebx, byte [eax]
  mov       [esi+ContextModel.st], ebx
  mov       eax, [WorkData+Work.stateMap+ebx*4]
  shr       eax, 4
  push      eax
  mov       eax, [WorkData+Work.stretch+eax*4]
  sar       eax, 2
  stosw
  pop       eax
  
  shr       eax, 4
  mov       edx, eax
  not       dl
  
  push      eax
  sub       eax, edx
  stosw
  pop       eax
  
  and       al, [WorkData+Work.stateCode+ebx*2+0]
  and       dl, [WorkData+Work.stateCode+ebx*2+1]
  sub       eax, edx
  stosw

  cmp       ebx, byte 1
  sbb       dword [ebp+Work.ctx+8], byte -1

  add       esi, byte ContextModel.size
  dec       ecx
  jnz       near .contextloop
  
  ; weighting contexts
  mov       ebx, [ebp+Work.dst]
  movzx     eax, byte [ebx-1]
  mov       [ebp+Work.ctx+0], eax
  mov       al, [ebp+Work.c0]
  inc       ah
  mov       [ebp+Work.ctx+4], eax
  
  xor       ebx, ebx
  mov       eax, [ebp+Work.matchl]
  dec       eax
  cmovs     eax, ebx
  not       bl
  cmp       eax, ebx
  cmova     eax, ebx
  mov       eax, [ebp+Work.runTable+eax*4]
  shr       eax, 3
  add       [ebp+Work.ctx+8], eax
  
.mix:
  ; perform mixing
  push      byte 2
  pop       ebx
  
.mixloop:
  mov       eax, [ebp+Work.ctx+ebx*4]
  imul      eax, byte (NINPUT*2)
  lea       edi, [ebp+Work.wx+eax]
  mov       esi, WorkData+Work.tx
    
  pxor      mm0, mm0
  push      byte (NINPUT/4)
  pop       ecx

.mixdploop:
  movq      mm1, [esi]
  pmaddwd   mm1, [edi]
  psrad     mm1, 8
  paddd     mm0, mm1
  add       esi, byte 8
  add       edi, byte 8
  loop      .mixdploop
  
  movq      mm1, mm0
  psrlq     mm1, 32
  paddd     mm0, mm1
  movd      eax, mm0
  sar       eax, 3

  mov       [WorkData+Work.tx2+ebx*2], ax
  call      squash
  mov       [ebp+Work.pr+ebx*4], eax
  
  dec       ebx
  jns       .mixloop
  
  ; the final mix
  mov       esi, WorkData+Work.tx2
  mov       edi, WorkData+Work.wx2
  xor       eax, eax
  push      byte 3
  pop       ecx
  
.finalmix:  
  movsx     ebx, word [edi]
  movsx     edx, word [esi]
  imul      ebx, edx
  add       eax, ebx
  cmpsw
  loop      .finalmix
  
  sar       eax, 16
  call      squash
  mov       [ebp+Work.pr+12], eax
  
%if USEAPM
  ; APM stage
  ; assumes p still in eax
  mov       ecx, [ebp+Work.bit]
  mov       edx, [ebp+Work.APMi]
  lea       edi, [ebp+Work.APM+16*4+edx*4]
  neg       ecx
  and       ecx, 0x100fe ;=g
  
  ; update APM tables
  push      ecx
  sub       ecx, [edi]
  sar       ecx, 8
  add       [edi], ecx
  add       edi, byte 4
  pop       ecx
  sub       ecx, [edi]
  sar       ecx, 8
  add       [edi], ecx
  
  ; calc new APM index
  mov       ebx, [ebp+Work.dst]
  movzx     ebx, byte [ebx-1]
  mov       ecx, ebx
  shl       ebx, 4
  add       ebx, ecx
  add       ebx, [ebp+Work.c0]
  and       ebx, APMSIZE-1
  mov       ecx, ebx
  shl       ebx, 5
  add       ebx, ecx

  ; calc pf/finish APMi
  mov       ecx, [ebp+Work.stretch+eax*4]
  push      ecx
  sar       ecx, 7
  add       ebx, ecx
  mov       [ebp+Work.APMi], ebx
  pop       ecx
  
  ; calc p
  and       ecx, byte 127
  lea       esi, [ebp+Work.APM+16*4+ebx*4]
  lodsd
  mov       ebx, [esi]
  sub       ebx, eax
  shl       eax, 7
  imul      ebx, ecx
  add       eax, ebx
  shr       eax, 11
  
  mov       [ebp+Work.outprob], eax
%endif  
  
  pop       ebx
  jmp       .decodebitloop  

; ---- initialized data

section       .data

squashTab     dw    1,   2,   4,   6,  10,  17,  27,  45,  74, 120, 194
              dw  311, 488, 747,1102,1546,2048,2550,2994,3349,3608,3785
              dw 3902,3976,4022,4051,4069,4079,4086,4090,4092,4094,4095
masks         db 0x1f, 0x27, 0x88, 0x07, 0x0a, 0x09, 0x05, 0x03, 0x04, 0x02, 0x01
bitm          db 0xff, 0xff, 0xff, 0xe0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff

