; This code is in the public domain. See LICENSE for details.

; ms2k invitation
;
; font stuff

%ifdef USEFONT

; -------------------------------------------- includes and definitions -----

bits          32

;%include      "win32.inc"

; ---------------------------------------------------- initialized data -----

section       .data

font:
  .weight  dw 700
  .size    db 22*4
  .name    db 'Arial',0,

; -------------------------------------------------- uninitialized data -----

section       .bss

BMI:
  .biSize           resd 1
  .biWidth          resd 1
  .biHeight         resd 1
  .biPlanes         resw 1
  .biBitCount       resw 1
  .biCompression    resd 1
  .biSizeImage      resd 1
  .biXPelsPerMeter  resd 1
  .biYPelsPerMeter  resd 1
  .biClrUsed        resd 1
  .biClrImportant   resd 1
  .bmiColors        resd 256

_bits         resd 1
char          resb 1

; -------------------------------------------- code section starts here -----

section       .text

; kernel32

extern        _LocalAlloc@8
extern        _LocalFree@4

; gdi32

extern        _CreateCompatibleDC@4
extern        _CreateDIBSection@24
extern        _CreateFontA@56
extern        _DeleteDC@4
extern        _DeleteObject@4
extern        _GdiFlush@0
extern        _GetCharABCWidthsA@16
extern        _SelectObject@8
extern        _SetBkMode@8
extern        _SetTextColor@8
extern        _TextOutA@20

global        _fontCreate@0
global        _fontDestroy@4

; ------------------------------------------------------- font erzeugen -----

_fontCreate@0:
  push        ebp
  push        edi
  push        esi
  push        ebx

  ; prepare bitmapinfo

  mov         edi, BMI
  xor         eax, eax
  mov         al, 40
  stosd
  xor         eax, eax
  mov         ah, 8
  stosd
  neg         eax
  stosd
  mov         eax, 0x00080001
  stosd
  xor         eax, eax
  stosd
  stosd
  stosd
  stosd
  inc         ah
  stosd
  stosd

  xor         eax, eax
  xor         ecx, ecx
  inc         ch
.palloop:
  stosd
  add         eax, 0x10101
  loop        .palloop

  ; create required GDI objects

  xor         ebx, ebx
  push        ebx
  call        _CreateCompatibleDC@4
  xchg        esi, eax                 ; esi=DC

  push        ebx
  push        ebx
  push        dword _bits
  push        ebx                      ; DIB_RGB_COLORS
  push        dword BMI
  push        esi
  call        _CreateDIBSection@24
  xchg        edi, eax                 ; edi=bitmap

  mov         ebp, font
  lea         eax, [ebp+font.name-font]
  push        eax
  push        ebx                      ; DEFAULT_PITCH
  lea         eax, [ebx+2]
  push        eax                      ; PROOF_QUALITY
  push        ebx                      ; CLIP_DEFAULT_PRECIS
  lea         eax, [ebx+7]
  push        eax                      ; OUT_TT_ONLY_PRECIS
  push        ebx                      ; ANSI_CHARSET
  push        ebx                      ; strikeout=0
  push        ebx                      ; underline=0
  push        ebx                      ; italic=0
  movzx       eax, word [ebp+font.weight-font]
  push        eax                      ; weight
  push        ebx                      ; orientation=0
  push        ebx                      ; escapement=0
  push        ebx                      ; width=0
  movzx       eax, byte [ebp+font.size-font]
  push        eax                      ; charsize
  call        _CreateFontA@56
  mov         ebx, eax                 ; ebx=font

  ; clear bits

  pushad
  mov         edi, [_bits]
  xor         eax, eax
  mov         ecx, 2048*2048/4
  rep         stosd
  popad

  ; gdi-rumgespacke

  push        ebx                      ; font
  push        esi                      ; DC
  call        _SelectObject@8

  push        edi                      ; bitmap
  push        esi                      ; DC
  call        _SelectObject@8

  push        dword 0xffffff
  push        esi                      ; DC
  call        _SetTextColor@8

  xor         eax, eax
  inc         eax
  push        eax                      ; TRANSPARENT
  push        esi                      ; DC
  call        _SetBkMode@8

  xor         eax, eax
  push        dword 262144+3072        ; sizeof(BitmapFont)
  push        eax                      ; LMEM_FIXED
  call        _LocalAlloc@8
  mov         ebp, eax                 ; ebp=fnt

  lea         eax, [ebp+262144]
  push        eax                      ; fnt->kerning
  xor         eax, eax
  dec         al
  push        eax                      ; 255
  xor         eax, eax
  push        eax                      ; 0
  push        esi                      ; DC
  call        _GetCharABCWidthsA@16

  ; font rendern

  xor         eax, eax
  mov         [char], al

  xor         edx, edx
.yloop:
  xor         ecx, ecx
.xloop:
  xor         eax, eax
  inc         eax

  pushad

  push        eax
  push        dword char
  push        edx
  push        ecx
  push        esi
  call        _TextOutA@20

  popad

  xor         eax, eax
  mov         al, 128
  inc         byte [char]
  add         ecx, eax
  cmp         ch, 8
  jnz         .xloop

  add         edx, eax
  cmp         dh, 8
  jnz         .yloop

  ; antialiasen+konvertieren

  pushad
  mov         edi, ebp

  xor         edx, edx
.ayloop:
  xor         ecx, ecx
.axloop:
  mov         esi, edx
  shl         esi, 11
  add         esi, ecx
  add         esi, [_bits]

  xor         ebx, ebx
  push        ecx

  xor         ecx, ecx
  mov         cl, 15
.collect:
  mov         eax, ecx
  shr         eax, 2
  shl         eax, 11
  add         eax, ecx
  and         al, 3

  movzx       eax, byte [esi+eax]
  add         ebx, eax
  dec         ecx
  jns         .collect

  pop         ecx

  shr         ebx, 4
  xchg        eax, ebx
  stosb

  xor         eax, eax
  mov         al, 4

  add         ecx, eax
  cmp         ch, 8
  jnz         .axloop

  add         edx, eax
  cmp         dh, 8
  jnz         .ayloop

  popad

  ; grossreinemachen

  push        edi                      ; map
  call        _DeleteObject@4
  push        ebx                      ; font
  call        _DeleteObject@4
  push        esi                      ; DC
  call        _DeleteDC@4

  mov         eax, ebp                 ; font returnen
  pop         ebx
  pop         esi
  pop         edi
  pop         ebp
  ret

; ------------------------------------------------------ font destroyen -----

_fontDestroy@4:                           ; eax=font
  jmp         _LocalFree@4

%endif
