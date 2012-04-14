;----------------------------------------------------------------------------
;
;                            E L I T E G R O U P
;                             we are very good.
;
; -> DirectSound output code
;
;----------------------------------------------------------------  ------------

SECTION .bss

%include    "win32.inc"
%include    "dsound.inc"

%define BUFFERLEN 10000h

struc dsdata
  .sbuf      resd 1             ; ptr to the secondary IDirectSoundBuffer
  .pbuf      resd 1             ; ptr to the primary   IDirectSoundBuffer
  .dssd      resd 1             ; ptr to the IDirectSound instance
  .callback  resd 1             ; ptr to the callback function
  .cbparm    resd 1             ; parm of the callback function
	.vol       resd 1             ; float: vol
  .exitreq   resd 1             ; exit request flag for sound thread
  .thndl     resd 1             ; handle of sound thread
  .buf1      resd 1             ; 1st locked buffer address for streaming
  .len1      resd 1             ; length of 1st buffer
  .buf2      resd 1             ; 2nd locked buffer address for streaming
  .len2      resd 1             ; length of 2nd buffer
  .towrite   resd 1             ; bytes to write
  .curpos    resd 1             ; current read position on buffer
  .lastpos   resd 1             ; last write position in buffer
  .dummy     resd 1             ; dummy dword for thread ID
  .tickev    resd 1             ; handle for tick event
	.cb1       resd 1             ; clamp val 1
	.crsec     resb 24            ; critical section
	.gppos     resd 2             ; pos 2
	.bufcnt    resd 1             ; filled buffer count
	.ltg       resd 1             ; last time got
  .tempwfx   resb WAVEFORMATEX.size
  .mixbuffer resb 2*BUFFERLEN   ; 32bit stereo mixing buffer
  .size
endstruc

extern _CreateThread@24
extern _WaitForSingleObject@8
extern _SetThreadPriority@8
extern _InitializeCriticalSection@4
extern _EnterCriticalSection@4
extern _LeaveCriticalSection@4
extern _DeleteCriticalSection@4
extern _DirectSoundCreate@12
extern _CreateEventA@16
extern _CloseHandle@4
extern _SetEvent@4

global _dsInit@12
global _dsClose@0
global _dsGetCurSmp@0
global _dsSetVolume@4
global _dsTick@0
global _dsLock@0
global _dsUnlock@0

globdat:     resb dsdata.size


section .drectve info
  db "/defaultlib:dsound.lib "
  db "/defaultlib:user32.lib "
  db "/defaultlib:kernel32.lib "
  db 0

SECTION .text


fc32768	dd 32768.0

; NEU
clamp;
		pushad
		shr ecx, 1
.lp
		fld		dword [esi]
		fmul  dword [ebx + dsdata.vol]
		add		esi, 4
		xor		edx, edx
		fistp dword [ebx + dsdata.cb1]
		mov		eax,  [ebx + dsdata.cb1]
    mov		dx, 32600
    cmp   eax, edx
    jng   .nocmax 
    mov   eax, edx
.nocmax 
		neg		edx
    cmp   eax, edx
    jnl   .clampend 
   	mov   eax, edx
.clampend
		stosw
		loop .lp
		popad
		ret



_dsInit@12:

    pushad
    mov      ebx, globdat

    ; clear global data
    mov      ecx, dsdata.size
    mov      edi, ebx
    xor      eax, eax
    rep      stosb

		mov      esi, [esp+36]
		mov      [ebx+dsdata.callback], esi
		mov      esi, [esp+40]
		mov      [ebx+dsdata.cbparm], esi
	
		lea      esi, [ebx+dsdata.dssd]
    mov      [esi], eax
		push     eax
		push     esi
		push     eax
		call     _DirectSoundCreate@12

		mov      esi, [esi]
		or       esi, esi
		jz			 .initfailgate
		

    ; set DSound cooperative level
    mov      al, DSSCL_PRIORITY
    push     eax
    push     dword [esp + 48]            ; second parm (hwnd)
    push     esi
    mov      edi, [esi]  ; edx = vtbl
    call     [edi + DirectSound8.SetCooperativeLevel]

    or       eax, eax
    jnz      .initfailgate

    ; obtain primary buffer
    push     eax
    lea      ebp, [ebx + dsdata.pbuf]
    push     ebp
    push     dword primdesc
    push     esi
    call     [edi + DirectSound8.CreateSoundBuffer]

    or       eax, eax
.initfailgate:
    jnz      .gate2


    ; obtain secondary buffer
    push     eax
    lea      edx, [ebx + dsdata.sbuf]
    push     edx
    push     dword streamdesc
    push     esi
    call     [edi + DirectSound8.CreateSoundBuffer]

    or       eax, eax
.gate2
    jnz      near .InitFailed


    ; set primary buffer format

    lea      edi, [ebx + dsdata.tempwfx]
    push     edi

    lea      esi, [wfxprimary]
    lea      ecx, [eax + WAVEFORMATEX.size]
    rep      movsb

    mov      esi, [ebp]
    push     esi
    mov      edi, [esi]  ; edx = vtbl
    call     [edi + DirectSoundBuffer8.SetFormat]


    ; lock, clear and unlock secondary buffer
    xor      esi,esi
    push     dword DSBLOCK_ENTIREBUFFER
    lea      edx, [ebx + dsdata.len2]
    push     edx
    lea      edx, [ebx + dsdata.buf2]
    push     edx
    lea      edx, [ebx + dsdata.len1]
    push     edx
    lea      edx, [ebx + dsdata.buf1]
    push     edx
    push     esi
    push     esi
    mov      ebp, [ebx + dsdata.sbuf]
    mov      esi, [ebp]    ; vtbl
    push     ebp
    call     [esi + DirectSoundBuffer8.Lock]
    or       eax, eax
    jnz      .gate2
    mov      ecx, [ebx + dsdata.len1]
    mov      edi, [ebx + dsdata.buf1]
    rep      stosb
    mov      ecx, [ebx + dsdata.len2]
    mov      edi, [ebx + dsdata.buf2]
    rep      stosb
    push     dword [ebx + dsdata.len2]
    push     dword [ebx + dsdata.buf2]
    push     dword [ebx + dsdata.len1]
    push     dword [ebx + dsdata.buf1]
    push     ebp
    call     [esi + DirectSoundBuffer8.Unlock]
    or       eax, eax
    jnz      .InitFailed

    mov      [ebx + dsdata.bufcnt], dword -BUFFERLEN
    mov      [ebx + dsdata.ltg], dword -BUFFERLEN

    ; tick event init
    xor      eax,eax
    push     eax    
    push     eax    
    push     eax    
    push     eax    
    call     _CreateEventA@16
    mov      [ebx + dsdata.tickev], eax

    ; critical section init...
    lea      eax, [ebx + dsdata.crsec]
    push     eax
    call     _InitializeCriticalSection@4

    ; activate the streaming buffer
    xor      eax,eax
    inc      al
    push     eax 
    push     dword DSBPLAY_LOOPING
    dec      al
    push     eax
    push     eax
    push     ebp
    call     [esi + DirectSoundBuffer8.Play]

    or       eax, eax
    jnz      .InitFailed

		; volume control init...
		fld      dword [fc32768]
		fstp     dword [ebx + dsdata.vol]

    ; start sound thread
    lea      edx, [ebx + dsdata.dummy]
    push     edx
    push     eax
    push     eax
    push     dword threadfunc
    push     eax
    push     eax
    call     _CreateThread@24
    mov      [ebx + dsdata.thndl], eax

    inc      dword [esp] ; <- THREAD_PRIORITY_HIGHER
    push     eax
    call     _SetThreadPriority@8

    ; ok, everything's done
    popad
    stc
    jmp     short .initends


.InitFailed   ; oh no, we've encountered an error!
    call     _dsClose@0
    popad
    clc
.initends
    sbb     eax,eax
    ret     0ch




_dsClose@0:

    pushad
    mov     ebx, globdat

    ; set exit request..
    inc     dword [ebx + dsdata.exitreq]
    mov     eax, [ebx + dsdata.thndl]
    or      eax, eax
    jz      .NoThread

    ; give the thread a chance to finish

    push    dword 1000
    push    eax
    call    _WaitForSingleObject@8

.NoThread:

    ;tick event release
    mov     eax, [ebx + dsdata.tickev]
    push    eax
    call    _CloseHandle@4

		; critical section release...
		lea      eax, [ebx + dsdata.crsec]
		push     eax
		call     _DeleteCriticalSection@4

    ; remove allocated instances
    mov     edi, .ReleaseComObj
    xchg    esi, ebx
    lodsd
    call    edi
    lodsd
    call    edi
    lodsd
    db  0b1h   ; 31337 untergrund trick!

.ReleaseComObj:
    pushad
    or      eax, eax
    jz      .endRelease
    mov     edx, [eax]
    push    eax
    call    [edx+DirectSound8.Release]
.endRelease

    popad
    ret


threadfunc:
    pushad
    mov     ebx, globdat

.looping:
    ; check for exit request
    cmp     byte [ebx + dsdata.exitreq], 0
    je      .loopok

    popad
    xor     eax,eax
    ret     4h

.loopok:
    xor     ebp,ebp
    lea     edx,[ebp+(BUFFERLEN/3000)] 
    push    edx
    push    dword [ebx + dsdata.tickev]
    call    _WaitForSingleObject@8

    mov     [ebx + dsdata.buf1], ebp
    mov     [ebx + dsdata.buf2], ebp

		lea      eax, [ebx + dsdata.crsec]
		push     eax
		call     _EnterCriticalSection@4

.lockLoop:
    ; fetch current buffer position
    xor     eax, eax
    push    eax
    lea     eax, [ebx + dsdata.curpos]
    push    eax             
    mov     ebp, [ebx + dsdata.sbuf]
    push    ebp
    mov     edx, [ebp]      ; vtbl
    call    [edx + DirectSoundBuffer8.GetCurrentPosition]
    cmp     eax, 88780096h
    je      .tryRestore

    ; find out how many bytes to write
    mov     eax, [ebx + dsdata.curpos]
    and     eax, ~1fh
    mov     ecx, eax
    sub     eax, [ebx + dsdata.lastpos]
		jnz     .fast
		jmp			.after
.fast
    jns     .ja
    add     eax, BUFFERLEN
.ja:
    mov     edi, eax
		add     [ebx + dsdata.bufcnt], eax

    ; try to lock the buffer
    xor     esi,esi
    push    esi
    lea     edx, [ebx + dsdata.len2]
    push    edx
    lea     edx, [ebx + dsdata.buf2]
    push    edx
    lea     edx, [ebx + dsdata.len1]
    push    edx
    lea     edx, [ebx + dsdata.buf1]
    push    edx

    push    eax
    push    dword [ebx + dsdata.lastpos]
    mov     [ebx + dsdata.lastpos],ecx

    mov     edx, [ebp]    ; vtbl
    push    ebp
    call    [edx + DirectSoundBuffer8.Lock]
    cmp     eax, 88780096h; DSERR_BUFFERLOST
    jne     .lockOK
.tryRestore:
    mov     edx, [ebp]    ; vtbl
    push    ebp
    call    [edx + DirectSoundBuffer8.Restore]
    jmp     .lockLoop

.lockOK:
    mov     eax, edi
    shr     eax, 2
		push    eax
		lea     edi, [ebx + dsdata.mixbuffer]
		push    edi
		push    dword [ebx + dsdata.cbparm]
    call    [ebx + dsdata.callback]

    mov     esi, edi
    push    ebp
    mov     ebp, clamp

    ; check buffer one
    mov     edi, [ebx + dsdata.buf1]
    or      edi, edi
    jz      .nobuf1
    mov     ecx, [ebx + dsdata.len1]
    call    ebp
    shl     ecx, 1
    add     esi, ecx

.nobuf1:
    ; check buffer two
    mov     edi, [ebx + dsdata.buf2]
    or      edi, edi
    jz      .nobuf2
    mov     ecx, [ebx + dsdata.len2]
    call    ebp

.nobuf2:
		fstp    st0
    pop     ebp
    push    dword [ebx + dsdata.len2]
    push    dword [ebx + dsdata.buf2]
    push    dword [ebx + dsdata.len1]
    push    dword [ebx + dsdata.buf1]
    push    ebp
    mov     esi, [ebp]     ; vtbl
    call    [esi + DirectSoundBuffer8.Unlock]

.after
		lea      eax, [ebx + dsdata.crsec]
		push     eax
		call     _LeaveCriticalSection@4

    jmp     .looping


_dsGetCurSmp@0:
    push    ebx
		push    ebp

		mov     ebx, globdat

		lea      eax, [ebx + dsdata.crsec]
		push     eax
		call     _EnterCriticalSection@4

	; fetch current buffer position
    xor     eax, eax
    push    eax
    lea     eax, [ebx + dsdata.gppos]
    push    eax             
    mov     ebp, [ebx + dsdata.sbuf]
    push    ebp
    mov     edx, [ebp]      ; vtbl
    call    [edx + DirectSoundBuffer8.GetCurrentPosition]

		lea      eax, [ebx + dsdata.crsec]
		push     eax
		call     _LeaveCriticalSection@4

		mov     eax, [ebx+dsdata.gppos]
		sub     eax, [ebx+dsdata.lastpos]
		jns     .noadd
    add     eax, BUFFERLEN
.noadd
		; WORKAROUNDMANIE!
    cmp     eax, BUFFERLEN/4
		jl      .isgut
		mov     eax, [ebx+dsdata.ltg]
		sub     eax, [ebx+dsdata.bufcnt]
.isgut
		add     eax, [ebx+dsdata.bufcnt]
		mov     [ebx+dsdata.ltg], eax

		pop			ebp
		pop			ebx
    ret

_dsSetVolume@4:
		push    ebx
		mov			ebx, globdat
		fld			dword [esp+8]
		fmul		dword [fc32768]
		fstp		dword [ebx + dsdata.vol]
		pop     ebx
		ret 4



_dsTick@0:
    push    ebx
		mov			ebx, globdat
    push    dword [ebx + dsdata.tickev]
    call    _SetEvent@4
		pop			ebx
		ret

_dsLock@0:
    push    ebx
    mov     ebx, globdat
		lea     eax, [ebx + dsdata.crsec]
		push    eax
		call    _EnterCriticalSection@4
		pop     ebx
		ret
		
_dsUnlock@0:
    push    ebx
    mov     ebx, globdat
		lea     eax, [ebx + dsdata.crsec]
		push    eax
		call    _LeaveCriticalSection@4
		pop     ebx
		ret
		
		

wfxprimary:
    dw  1										; wFormatTag: WAVEFORMAT_PCM
		dw  2                   ; nChannels
    dd  44100 		          ; nSamplesPerSec: 44100
		dd  176400              ; nAvgBytesPerSec
		dw  4                   ; nBlockAlign
    dw  16                  ; wBitsPerSample
    dw  0                   ; cbSize

streamdesc:
    dd	streamdesc.size-streamdesc
		dd  DSBCAPS_GETCURRENTPOSITION2|DSBCAPS_GLOBALFOCUS
		dd  BUFFERLEN						; dwBufferBytes
    dd  0										; dwReserved
    dd  wfxprimary          ; lpwfxFormat
.size

primdesc:
		dd primdesc.size-primdesc
		dd DSBCAPS_PRIMARYBUFFER
		dd 0
		dd 0
		dd 0
.size
