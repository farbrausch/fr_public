; This file is distributed under a BSD license. See LICENSE.txt for details.

;#####################################################################################
;
;
; Farb-Rausch ViruZ II  -  wer das liest, ist rule
;
;
;#####################################################################################


%define		POLY			32  							 ; maximum polyphony
%define		LOWEST		39000000h					 ; out of 16bit range
%define		MDMAXOFFS 1024

%define   MAX_FRAME_SIZE 280

;%define   VUMETER

%define   FIXDENORMALS 1               ; there's really no good reason to turn this off.
%define   V2PROFILER 0                 ; profiling support. refer to tinyplayer
%define		RONAN
%define   FULLMIDI

icnoisemul equ 196314165
icnoiseadd equ 907633515

section .bss

maindelbuf  resd 2*32768
chandelbuf  resd 16*2*2048

vcebuf			resd MAX_FRAME_SIZE
vcebuf2			resd MAX_FRAME_SIZE
chanbuf			resd 2*MAX_FRAME_SIZE
aux1buf			resd MAX_FRAME_SIZE
aux2buf			resd MAX_FRAME_SIZE
mixbuf			resd 2*MAX_FRAME_SIZE
auxabuf			resd 2*MAX_FRAME_SIZE
auxbbuf			resd 2*MAX_FRAME_SIZE


temp				resd 16
oldfpcw			resd 1

section .text

global _AUX_
_AUX_

fci12			     dd 0.083333333333
fc2            dd 2.0
fc3            dd 3.0
fc6            dd 6.0
fc15           dd 15.0
fc32bit        dd 2147483647.0
fci127				 dd 0.007874015748
fci128         dd 0.0078125
fci4           dd 0.25
fci2           dd 0.5
fcpi_2				 dd 1.5707963267948966192313216916398
fcpi           dd 3.1415926535897932384626433832795
fc1p5pi        dd 4.7123889803846898576939650749193
fc2pi					 dd	6.28318530717958647692528676655901
fc64           dd 64.0
fc32           dd 32.0
fci16					 dd 0.0625
fcipi_2				 dd 0.636619772367581343075535053490057
fc32768        dd 32768.0
fc256          dd 256.0
fc10           dd 10.0
fci32768       dd 0.000030517578125
fc1023         dd 1023.0
fcm12          dd -12.0
fcm16          dd -16.0
fci8192        dd 0.0110485434560398050687631931578883
fci64          dd 0.015625
fc48           dd 48.0

fciframe			 dd 0.0078125     ; 1/FRAME_SIZE
fccfframe      dd 11.0          ; for calcfreq2

fcOscPitchOffs dd 60.0
;fcfmmax        dd 4.0
fcfmmax        dd 2.0
fcattackmul    dd -0.09375 ;-0.0859375
fcattackadd    dd 7.0
fcsusmul       dd 0.0019375
fcgain         dd 0.6
fcgainh        dd 0.6
fcmdlfomul     dd 1973915.486621315192743764172335601
fcsamplesperms dd 44.1
fccrelease     dd 0.01
fccpdfalloff   dd 0.9998


%ifdef VUMETER
fcvufalloff    dd 0.9999
%endif

fcsinx3        dq -0.16666
fcsinx5        dq 0.0083143
fcsinx7        dq -0.00018542


; r(x) = (x + 0.43157974*x^3)/(1 + 0.76443945*x^2 + 0.05831938*x^4)
; second set of coeffs is for r(1/x)
fcatanx1       dq 1.0         , -0.431597974
fcatanx3       dq 0.43157974  , -1.0
fcatanxm0      dq 1.0         , 0.05831938
fcatanxm4      dq 0.05831938  , 1.0
fcatanxm2      dq 0.76443945
fcatanadd      dq 0.0         , 1.5707963267948966192313216916398



section .data
SRfcobasefrq		dd 12740060.0   ; C-3: 130.81278265029931733892499676165 Hz * 2^32 / SR
SRfclinfreq			dd 1.0                                   ; 44100hz/SR
SRfcBoostSin		dd 0.0213697517873015514981556266069164  ; sin of 150hz/SR
SRfcBoostCos		dd 0.999771640780307951892102701484347   ; cos of 150hz/SR
SRcFrameSize    dd 128

fcoscbase       dd 261.6255653005986346778499935233
fcsrbase        dd 44100.0
fcboostfreq     dd 150.0
fcframebase     dd 128.0

dcoffset        dd 0.000003814697265625 ; 2^-18

;#####################################################################################
;  Performance Monitor
;#####################################################################################

%define V2Perf_TOTAL      0
%define V2Perf_OSC        1
%define V2Perf_VCF        2
%define V2Perf_DIST       3
%define V2Perf_BASS       4
%define V2Perf_MODDEL     5
%define V2Perf_COMPRESSOR 6
%define V2Perf_PARAEQ     7
%define V2Perf_REVERB     8
%define V2Perf_RONAN      9

%define V2Perf_MAX        10

%if V2PROFILER
global          _V2Perf
_V2Perf:
  ;   1234567890123456
  db "Total           ",0,0,0,0,0,0,0,0
  db "Oscillators     ",0,0,0,0,0,0,0,0
  db "Filters         ",0,0,0,0,0,0,0,0
  db "Distortion      ",0,0,0,0,0,0,0,0
  db "Bass Boost      ",0,0,0,0,0,0,0,0
  db "ModDelay        ",0,0,0,0,0,0,0,0
  db "Compressor      ",0,0,0,0,0,0,0,0
  db "Equalizer       ",0,0,0,0,0,0,0,0
  db "Reverb          ",0,0,0,0,0,0,0,0
  db "Ronan           ",0,0,0,0,0,0,0,0
  times 24 db 0
  
V2PerfCounter:
  times V2Perf_MAX dd 0,0  
%endif

%macro V2PerfEnter 1
  %if V2PROFILER
    push    eax
    push    edx
    rdtsc
    sub     dword [V2PerfCounter + %1*8], eax
    sbb     dword [V2PerfCounter + %1*8 + 4], edx
    pop     edx
    pop     eax
  %endif
%endmacro

%macro V2PerfLeave 1
  %if V2PROFILER
    push    eax
    push    edx
    rdtsc
    add     dword [V2PerfCounter + %1*8], eax
    adc     dword [V2PerfCounter + %1*8 + 4], edx
    pop     edx
    pop     eax
  %endif
%endmacro

%macro V2PerfClear 0
  %if V2PROFILER
    push    edi
    push    eax
    push    ecx
    mov     edi, V2PerfCounter
    mov     ecx, V2Perf_MAX * 2
    xor     eax, eax
    rep     stosd
    pop     ecx
    pop     eax
    pop     edi
  %endif
%endmacro

%macro V2PerfCopy 0
  %if V2PROFILER
    push    esi
    push    edi
    mov     esi, V2PerfCounter
    mov     edi, _V2Perf
  %%lp:
    cmp     byte [edi], 0
    je      %%end
    add     edi, byte 16
    movsd
    movsd
    jmp     short %%lp
  %%end:
    pop     edi
    pop     esi
  %endif
%endmacro

;#####################################################################################
;  Helper Routines    
;#####################################################################################

section .text

global fastatan

fastatan: ; fast atan
          ; value in st0, -1 in st1, high 16bits in ax, "8" in ebx
          
	shl			ax, 1
	fld1											; <1> <val> <-1>
	fcmovb  st0, st2          ; <sign> <val> <-1>
	
	xor     edx, edx
	cmp     ah, 7fh
	fmul    st1, st0          ; <sign> <absval> <-1>
	cmovge  edx, ebx
	fxch    st1               ; <x> <sign> <-1>
	
	; r(x)= (cx1*x + cx3*x^3)/(cxm0 + cxm2*x^2 + cxm4*x^4)
	fld     st0                       ; <x> <x> <sign> <-1>
	fmul    st0, st0                  ; <x²> <x> <sign> <-1>
	fld     st0                       ; <x²> <x²> <x> <sign> <-1>
	fld     st0                       ; <x²> <x²> <x²> <x> <sign> <-1>
	fmul    qword [fcatanx3 + edx]    ; <x²*cx3> <x²> <x²> <x> <sign> <-1>
	fxch    st1                       ; <x²> <x²(cx3)> <x²> <x> <sign> <-1>
	fmul    qword [fcatanxm4 + edx]   ; <x²(cxm4)> <x²(cx3)> <x²> <x> <sign> <-1>
	fxch    st1                       ; <x²(cx3)> <x²(cxm4)> <x²> <x> <sign> <-1>
	fadd    qword [fcatanx1 + edx]    ; <cx1+x²(cx3)> <x²(cxm4)> <x²> <x> <sign> <-1>
	fxch    st1                       ; <x²(cxm4)> <cx1+x²(cx3)> <x²> <x> <sign> <-1>
	fadd    qword [fcatanxm2]         ; <cxm2+x²(cxm4)> <cx1+x²(cx3)> <x²> <x> <sign> <-1>
	fxch    st1                       ; <cx1+x²(cx3)> <cxm2+x²(cxm4)> <x²> <x> <sign> <-1>
	fmulp   st3, st0                  ; <cxm2+x²(cxm4)> <x²> <x(cx1)+x³(cx3)> <sign> <-1>
	fmulp   st1, st0                  ; <x²(cxm2)+x^4(cxm4)> <x(cx1)+x³(cx3)> <sign> <-1>
	fadd    qword [fcatanxm0 + edx]   ; <cxm0+x²(cxm2)+x^4(cxm4)> <x(cx1)+x³(cx3)> <sign> <-1>
	fdivp   st1, st0                  ; <r(x)> <sign> <-1>
	fadd    qword [fcatanadd + edx]   ; <r(x)+adder) <sign> <-1>

	fmulp   st1, st0                  ; <sign*r'(x)> <-1>
	ret


global fastsinrc
global fastsin

; x+ax3+bx5+cx7
; ((((c*x²)+b)*x²+a)*x²+1)*x


fastsinrc: ; fast sinus with range check
  fld			dword [fc2pi]            ; <2pi> <x>
  fxch		st1                     ; <x> <2pi>
  fprem		                        ; <x'> <2pi>
  fxch		st1                     ; <2pi> <x'>
  fstp		st0                     ; <x'>
 
  fld1                            ; <1> <x>
  fldz                            ; <0> <1> <x> 
  fsub    st0, st1								; <mul> <1> <x>
  fldpi                           ; <sub> <mul> <1> <x>
  
  fld     dword [fcpi_2]          ; <pi/2> <sub> <mul> <1> <x>
  fcomi   st0, st4                
  fstp    st0                     ; <sub> <mul> <1> <x>
  fldz                            ; <0> <sub> <mul> <1> <x>
  fxch    st1                     ; <sub> <0> <mul> <1> <x>
  fcmovnb st0, st1                ; <sub'> <0> <mul> <1> <x>
  fxch    st1                     ; <0> <sub'> <mul> <1> <x>
  fstp    st0                     ; <sub'> <mul> <1> <x>
  fxch    st1                     ; <mul> <sub'> <1> <x>
  fcmovnb st0, st2                ; <mul'> <sub'> <1> <x>
  
  fld     dword [fc1p5pi]         ; <1.5pi> <mul'> <sub'> <1> <x>
  fcomi   st0, st4               
  fstp    st0                     ; <mul'> <sub'> <1> <x>
  fld     dword [fc2pi]           ; <2pi> <mul'> <sub'> <1> <x>
  fxch    st1                     ; <mul'> <2pi> <sub'> <1> <x>
  fcmovb  st0, st3                ; <mul''> <2pi> <sub'> <1> <x>
  fxch    st2                     ; <sub'> <2pi> <mul''> <1> <x>
  fcmovb  st0, st1                ; <sub''> <2pi> <mul''> <1> <x>
  fsubp   st4, st0                ; <2pi> <mul''> <1> <x-sub>
  fstp    st0                     ; <mul''> <1> <x-sub>
  fmulp   st2, st0                ; <1> <mul(x-sub)>
  fstp    st0                     ; <mul(x-sub)>
  
            
fastsin: ; fast sinus approximation (st0 -> st0) from -pi/2 to pi/2, about -80dB error, should be ok
  fld		st0											  ; <x> <x>
  fmul	st0, st1                 ; <x²> <x>
  fld		qword [fcsinx7]           ; <c> <x²> <x>
  fmul	st0, st1                 ; <cx²> <x²> <x>
  fadd	qword [fcsinx5]          ; <b+cx²> <x²> <x>
  fmul  st0, st1                 ; <x²(b+cx²)> <x²> <x>
  fadd  qword [fcsinx3]          ; <a+x²(b+cx²)> <x²> <x>
  fmulp st1, st0                 ; <x²(a+x²(b+cx²)> <x>
  fld1                           ; <1> <x²(a+x²(b+cx²)> <x>
  faddp st1, st0                 ; <1+x²(a+x²(b+cx²)> <x>
  fmulp st1, st0                 ; <x(1+x²(a+x²(b+cx²))>
  ret
  
  

global calcNewSampleRate
calcNewSampleRate: ; new SR in eax
	mov			[temp], eax
	fild			dword [temp]					; <sr>
	fld1													; <1> <sr>
	fdiv    st0, st1							; <1/sr> <sr>
	fld     dword [fcoscbase]			; <oscbHz> <1/sr> <sr>
	fmul    dword [fc32bit]				; <oscb32>  <1/sr> <sr>
	fmul    st0, st1							; <oscbout> <1/sr> <sr>
	fstp    dword [SRfcobasefrq]  ; <1/sr> <sr>
	fld     dword [fcsrbase]      ; <srbase> <1/sr> <sr>
	fmul    st0, st1              ; <linfrq> <1/sr> <sr>
	fstp    dword [SRfclinfreq]		; <1/sr> <sr>
	fmul    dword [fc2pi]         ; <boalpha> <sr>
	fmul    dword [fcboostfreq]   ; <bof/sr> <sr>
	fsincos                       ; <cos> <sin> <sr>
	fstp    dword [SRfcBoostCos]  ; <sin> <sr>
	fstp    dword [SRfcBoostSin]  ; <sr>
	fmul    dword [fcframebase]   ; <framebase*sr>
	fdiv    dword [fcsrbase]      ; <framelen>
	fadd    dword [fci2]          ; <round framelen>
	fistp   dword [SRcFrameSize]  ; -
	ret


calcfreq2:
    fld1									;	 1 <0..1>
		fsubp		st1, st0			;  <-1..0>
		fmul    dword [fccfframe]  ;  <-11..0> -> <1/2048..1>

		fld1
		fld	st1
		fprem
		f2xm1
		faddp		st1, st0
		fscale
		fstp	st1
		ret

global calcfreq2
global calcfreq
global pow2
global pow

calcfreq: ; (0..1 linear zu 2^-10..1)
    fld1									;	 1 <0..1>
		fsubp		st1, st0			;  <-1..0>
		fmul    dword [fc10]  ;  <-10..0> -> <1/1024..1>

		fld1
		fld	st1
		fprem
		f2xm1
		faddp		st1, st0
		fscale
		fstp	st1
		ret

pow2:

		fld1
		fld	st1
		fprem
		f2xm1
		faddp		st1, st0
		fscale
		fstp	st1
		ret

pow:
		fyl2x
		fld1
		fld	st1
		fprem
		f2xm1
		faddp		st1, st0
		fscale
		fstp	st1
		ret


;#####################################################################################
;  Oszillator
;#####################################################################################


global _OSC_
_OSC_

struc syVOsc
	.mode			resd 1  
  .ring     resd 1
	.pitch    resd 1
	.detune   resd 1
	.color    resd 1
	.gain     resd 1
	.size
endstruc

struc syWOsc
	.mode			resd 1  ; dword: mode (0=tri/saw 1=pulse 2=sin 3=noise)
  .ring     resd 1  ; dword: ringmod on/off
	.cnt      resd 1  ; dword: wave counter (32bit)
	.freq     resd 1  ; dword: wave counter inc (8x/sample)
	.brpt     resd 1  ; dword: break point für tri/pulse (32bit)
	.nffrq    resd 1  ; noise: filter freq
	.nfres    resd 1  ; noise: filter reso
	.nseed    resd 1  ; noise: random seed
	.gain     resd 1  ; output gain
	.gain4    resd 1  ; output gain (oversampled)
	.tmp      resd 1  ; temp
	.nfl      resd 1
	.nfb      resd 1
	.note     resd 1  ; note
	.pitch    resd 1  ; note
	.size
endstruc


syOscInit:
		pushad
		xor			eax, eax
		mov     [ebp + syWOsc.cnt], eax
		mov     [ebp + syWOsc.cnt], eax
		mov     [ebp + syWOsc.nfl], eax
		mov     [ebp + syWOsc.nfb], eax
		
		rdtsc
		mov     [ebp + syWOsc.nseed], eax
		popad
		ret



syOscChgPitch:
	  fld     dword [ebp + syWOsc.pitch]
		fld     st0
		fadd    dword [fc64]
		fmul    dword [fci128]
		call    calcfreq
		fmul    dword [SRfclinfreq]
		fstp    dword [ebp + syWOsc.nffrq]
	  fadd    dword [ebp + syWOsc.note]
		fsub    dword [fcOscPitchOffs]
		fmul    dword [fci12]
		call    pow2
		fmul    dword [SRfcobasefrq]
		fistp   dword [ebp + syWOsc.freq]
		ret



syOscSet:
		pushad
		fld     dword [esi + syVOsc.mode]
		fistp   dword [ebp + syWOsc.mode]

		fld     dword [esi + syVOsc.ring]
		fistp   dword [ebp + syWOsc.ring]

	  fld     dword [esi + syVOsc.pitch]
		fsub    dword [fc64]
		fld			dword [esi + syVOsc.detune]
		fsub    dword [fc64]
		fmul    dword [fci128]
		faddp   st1, st0
		fstp    dword [ebp + syWOsc.pitch]
		call    syOscChgPitch

		fld     dword [esi + syVOsc.gain]
		fmul    dword [fci128]
		fst     dword [ebp + syWOsc.gain]
		fmul    dword [fci4]
		fstp    dword [ebp + syWOsc.gain4]

		fld     dword [esi + syVOsc.color] ; <c>
		fmul    dword [fci128]						 ; <c'>
		fld     st0                        ; <c'> <c'>
		fmul    dword [fc32bit]						 ; <bp> <c'> 
		fistp   dword [ebp + syWOsc.brpt]  ; <c'>
		shl     dword [ebp + syWOsc.brpt],1 

		fsqrt                              ; <rc'>
		fld1                               ; <1> <rc'>
		fsubrp  st1, st0                   ; <1-rc'>
		fstp    dword [ebp + syWOsc.nfres] ; -
		
		popad
		ret


; edi: dest buf
; esi: source buf
; ecx: # of samples
; ebp: workspace

section .data

oscjtab dd syOscRender.off, syOscRender.mode0, syOscRender.mode1, syOscRender.mode2, 
				dd syOscRender.mode3, syOscRender.mode4, syOscRender.auxa, syOscRender.auxb

section .text

syOscRender:
		pushad
		V2PerfEnter V2Perf_OSC
		lea     edi, [edi + 4*ecx]
		neg     ecx
		movzx		eax, byte [ebp + syWOsc.mode]
		and			al, 7
		jmp			dword [oscjtab + 4*eax]

section .data

.m0casetab   dd syOscRender.m0c2,     ; ... 
             dd syOscRender.m0c1,     ; ..n , INVALID!
             dd syOscRender.m0c212,   ; .c.
             dd syOscRender.m0c21,    ; .cn
             dd syOscRender.m0c12,    ; o..
             dd syOscRender.m0c1,     ; o.n
             dd syOscRender.m0c2,     ; oc. , INVALID!
             dd syOscRender.m0c121    ; ocn

section .text

.mode0     ; tri/saw
		mov		eax, [ebp + syWOsc.cnt]
    mov   esi, [ebp + syWOsc.freq]
 
    ; calc float helper values
		mov   ebx, esi
		shr   ebx, 9
		or    ebx, 0x3f800000							
		mov   [ebp + syWOsc.tmp], ebx
    fld   dword [ebp + syWOsc.gain]     ; <g>
    fld1                                ; 1 <g>
    fsubr dword [ebp + syWOsc.tmp]      ; <f> <g>
    fld1                                ; <1> <f> <g>
    fdiv  st0, st1                      ; <1/f> <f> <g>
    fld   st2                           ; <g> <1/f> <f> <g>
 		mov   ebx, [ebp + syWOsc.brpt]
		shr   ebx, 9
		or    ebx, 0x3f800000							
		mov   [ebp + syWOsc.tmp], ebx
    fld   dword [ebp + syWOsc.tmp]     ; <b> <g> <1/f> <f> <g>
    fld1                               ; <1> <b> <g> <1/f> <f> <g>
    fsubr  st0, st1                    ; <col> <b> <g> <1/f> <f> <g>
    
    ; m1=2/col
    ; m2=-2/(1-col)
    ; c1=gain/2*m1 = gain/col
    ; c2=gain/2*m2 = -gain/(1-col)
    fld    st0                         ; <col> <col> <b> <g> <1/f> <f> <g>
		fdivr  st0, st3                    ; <c1> <col> <b> <g> <1/f> <f> <g>
    fld1                               ; <1> <c1> <col> <b> <g> <1/f> <f> <g>
    fsubrp st2, st0                    ; <c1> <1-col> <b> <g> <1/f> <f> <g>
    fxch   st1                         ; <1-col> <c1> <b> <g> <1/f> <f> <g>
    
    fdivp  st3, st0                    ; <c1> <b> <g/(1-col)> <1/f> <f> <g>
    fxch   st2                         ; <g/(1-col)> <b> <c1> <1/f> <f> <g>
    fchs                               ; <c2> <b> <c1> <1/f> <f> <g>

    ; calc state
    mov   ebx, eax
    sub   ebx, esi                 ; ................................  c
    rcr   edx, 1                   ; c...............................  .
    cmp   ebx, [ebp + syWOsc.brpt] ; c...............................  n
    rcl   edx, 2                   ; ..............................nc  .


.m0loop
  		mov   ebx, eax
  		shr   ebx, 9
  		or    ebx, 0x3f800000							
  		mov   [ebp + syWOsc.tmp], ebx

      fld   dword [ebp + syWOsc.tmp] ; <p+b> <c2> <b> <c1> <1/f> <f> <g>
      fsub  st0, st2                 ; <p> <c2> <b> <c1> <1/f> <f> <g>
      cmp   eax, [ebp + syWOsc.brpt] ; ..............................oc  n
      rcl   edx, 1                   ; .............................ocn  .
      and   edx, 7                   ; 00000000000000000000000000000ocn
  		jmp		dword [cs: .m0casetab + 4*edx]

; cases: on entry <p> <c2> <b> <c1> <1/f> <f>, on exit: <y> <c2> <b> <c1> <1/f> <f>

.m0c21 ; y=-(g+c2(p-f+1)²-c1p²)*(1/f)
      fld1                             ; <1> <p> <c2> <b> <c1> <1/f> <f> <g>
      fadd   st0, st1                  ; <p+1> <p> <c2> <b> <c1> <1/f> <f> <g>
      fsub   st0, st6                  ; <p+1-f> <p> <c2> <b> <c1> <1/f> <f> <g>
      fmul   st0, st0                  ; <(p+1-f)²> <p> <c2> <b> <c1> <1/f> <f> <g>
      fmul   st0, st2                  ; <c2(p+1-f)²> <p> <c2> <b> <c1> <1/f> <f> <g>
      fxch   st1                       ; <p> <c2(p+1-f)²> <c2> <b> <c1> <1/f> <f> <g>
      fmul   st0, st0                  ; <p²> <c2(p+1-f)²> <c2> <b> <c1> <1/f> <f> <g>
      fmul   st0, st4                  ; <c1p²> <c2(p+1-f)²> <c2> <b> <c1> <1/f> <f> <g>
      fsubp  st1, st0                  ; <c2(p-f+1)²-c1p²> <c2> <b> <c1> <1/f> <f> <g>
      fadd   st0, st6                  ; <g+c2(p-f+1)²-c1p²> <c2> <b> <c1> <1/f> <f> <g>
      fchs                             ; <-(g+c2(p-f+1)²-c1p²)> <c2> <b> <c1> <1/f> <f> <g>
      fmul   st0, st4                  ; <y> <c2> <b> <c1> <1/f> <f> <g>
      jmp    short .m0pl


.m0c121 ; y=(-g-c1(1-f)(2p+1-f))*(1/f)
      fadd   st0, st0                  ; <2p> <c2> <b> <c1> <1/f> <f> <g>
      fld1                             ; <1> <2p> <c2> <b> <c1> <1/f> <f> <g>
      fsub   st0, st6                  ; <1-f> <2p> <c2> <b> <c1> <1/f> <f> <g>
      fxch   st1                       ; <2p> <1-f> <c2> <b> <c1> <1/f> <f> <g>
      fadd   st0, st1                  ; <2p+1-f> <1-f> <c2> <b> <c1> <1/f> <f> <g>
      fmulp  st1, st0                  ; <(1-f)(2p+1-f)> <c2> <b> <c1> <1/f> <f> <g>
      fmul   st0, st3                  ; <c1(1-f)(2p+1-f)> <c2> <b> <c1> <1/f> <f> <g>
      fadd   st0, st6                  ; <g+c1(1-f)(2p+1-f)> <c2> <b> <c1> <1/f> <f> <g>
      fchs                             ; <-g-c1(1-f)(2p+1-f)> <c2> <b> <c1> <1/f> <f> <g>
      fmul   st0, st4                  ; <y> <c2> <b> <c1> <1/f> <f> <g>
      jmp    short .m0pl

      
.m0c212 ; y=(g-c2(1-f)(2p+1-f))*(1/f)
      fadd   st0, st0                  ; <2p> <c2> <b> <c1> <1/f> <f> <g>
      fld1                             ; <1> <2p> <c2> <b> <c1> <1/f> <f> <g>
      fsub   st0, st6                  ; <1-f> <2p> <c2> <b> <c1> <1/f> <f> <g>
      fxch   st1                       ; <2p> <1-f> <c2> <b> <c1> <1/f> <f> <g>
      fadd   st0, st1                  ; <2p+1-f> <1-f> <c2> <b> <c1> <1/f> <f> <g>
      fmulp  st1, st0                  ; <(1-f)(2p+1-f)> <c2> <b> <c1> <1/f> <f> <g>
      fmul   st0, st1                  ; <c2(1-f)(2p+1-f)> <c2> <b> <c1> <1/f> <f> <g>
      fadd   st0, st6                  ; <g+c2(1-f)(2p+1-f)> <c2> <b> <c1> <1/f> <f> <g>
      fchs                             ; <-g-c2(1-f)(2p+1-f)> <c2> <b> <c1> <1/f> <f> <g>
      fmul   st0, st4                  ; <y> <c2> <b> <c1> <1/f> <f> <g>
      jmp    short .m0pl

.m0c12  ; y=(c2(p²)-c1((p-f)²))*(1/f)
      fld   st0                       ; <p> <p> <c2> <b> <c1> <1/f> <f> <g>
      fmul  st0, st0                  ; <p²> <p> <c2> <b> <c1> <1/f> <f> <g>
      fmul  st0, st2                  ; <c2(p²)> <p> <c2> <b> <c1> <1/f> <f> <g>
      fxch  st1                       ; <p> <c2(p²)> <c2> <b> <c1> <1/f> <f> <g>
      fsub  st0, st6                  ; <p-f> <c2(p²)> <c2> <b> <c1> <1/f> <f> <g>
      fmul  st0, st0                  ; <(p-f)²> <c2(p²)> <c2> <b> <c1> <1/f> <f> <g>
      fmul  st0, st4                  ; <c1*(p-f)²> <c2(p²)> <c2> <b> <c1> <1/f> <f> <g>
      fsubp st1, st0                  ; <c2(p²)-c1*(p-f)²> <c2> <b> <c1> <1/f> <f> <g>
      fmul  st0, st4                  ; <y> <c2> <b> <c1> <1/f> <f> <g>
      jmp   short .m0pl

.m0c1  ; y=c1(2p-f)
      fadd  st0, st0                  ; <2p> <c2> <b> <c1> <1/f> <f> <g>
      fsub  st0, st5                  ; <2p-f> <c2> <b> <c1> <1/f> <f> <g>
      fmul  st0, st3                  ; <y> <c2> <b> <c1> <1/f> <f> <g>
      jmp   short .m0pl

.m0c2  ; y=c2(2p-f)
      fadd  st0, st0                  ; <2p> <c2> <b> <c1> <1/f> <f> <g>
      fsub  st0, st5                  ; <2p-f> <c2> <b> <c1> <1/f> <f> <g>
      fmul  st0, st1                  ; <y> <c2> <b> <c1> <1/f> <f> <g>

.m0pl
      fadd  st0, st6                 ; <out> <c2> <b> <c1> <1/f> <f> <g>
      add   eax, esi                 ; ...............................n  c
      rcl   edx, 1                   ; ..............................nc  .
      test  byte [ebp + syWOsc.ring], 1
      jz    .m0noring
  	  fmul	dword [edi + 4*ecx]      ; <out'> <c2> <b> <c1> <1/f> <f> <g>
      jmp   .m0store
.m0noring
		  fadd	dword [edi + 4*ecx]      ; <out'> <c2> <b> <c1> <1/f> <f> <g>
.m0store
		  fstp	dword [edi + 4*ecx]      ; <c2> <b> <c1> <1/f> <f> <g>
		  inc		ecx
		jz   .m0end
    jmp  .m0loop
.m0end
    mov   [ebp + syWOsc.cnt], eax
    fstp  st0 ; <b> <c1> <1/f> <f> <g>
    fstp  st0 ; <c1> <1/f> <f> <g>
    fstp  st0 ; <1/f> <f> <g>
    fstp  st0 ; <f> <g>
    fstp  st0 ; <g>
    fstp  st0 ; -
.off
    V2PerfLeave V2Perf_OSC
		popad				
    ret







.m1casetab   dd syOscRender.m1c2,     ; ... 
             dd syOscRender.m1c1,     ; ..n , INVALID!
             dd syOscRender.m1c212,   ; .c.
             dd syOscRender.m1c21,    ; .cn
             dd syOscRender.m1c12,    ; o..
             dd syOscRender.m1c1,     ; o.n
             dd syOscRender.m1c2,     ; oc. , INVALID!
             dd syOscRender.m1c121    ; ocn

.mode1     ; pulse
		mov		eax, [ebp + syWOsc.cnt]
    mov   esi, [ebp + syWOsc.freq]
 
    ; calc float helper values
		mov   ebx, esi
		shr   ebx, 9
		or    ebx, 0x3f800000							
		mov   [ebp + syWOsc.tmp], ebx
    fld   dword [ebp + syWOsc.gain]   ; <g>
    fld   dword [ebp + syWOsc.tmp]    ; <f+1> <g>
    fld1                              ; <1> <f+1> <g>
    fsubp st1, st0                    ; <f> <g>
    fdivr st0, st1                    ; <gdf> <g>
		mov   ebx, [ebp + syWOsc.brpt]
		shr   ebx, 9
		or    ebx, 0x3f800000							
		mov   [ebp + syWOsc.tmp], ebx
    fld   dword [ebp + syWOsc.tmp]    ; <b> <gdf> <g>
    fld   st0                         ; <b> <b> <gdf> <g>
    fadd  st0, st0                    ; <2b> <b> <gdf> <g>
    fld   st0                         ; <2b> <2b> <b> <gdf> <g>
    fld1                              ; <1> <2b> <2b> <b> <gdf> <g>
    fadd  st0, st0                    ; <2> <2b> <2b> <b> <gdf> <g>
    fsub  st1, st0                    ; <2> <2b-2> <2b> <b> <gdf> <g>
    fadd  st0, st0                    ; <4> <2b-2> <2b> <b> <gdf> <g>
    fsubp st2, st0                    ; <2b-2> <2b-4> <b> <gdf> <g>
    fmul  st0, st3                    ; <gdf(2b-2)> <2b-4> <b> <gdf> <g>
    fsub  st0, st4                    ; <cc212> <2b-4> <b> <gdf> <g>
    fxch  st1                         ; <2b-4> <cc212> <b> <gdf> <g>
    fmul  st0, st3                    ; <gdf(2b-4)> <cc212> <b> <gdf> <g>
    fadd  st0, st4                    ; <cc121> <cc212> <b> <gdf> <g>


    ; calc state
    mov   ebx, eax
    sub   ebx, esi                 ; ................................  c
    rcr   edx, 1                   ; c...............................  .
    cmp   ebx, [ebp + syWOsc.brpt] ; c...............................  n
    rcl   edx, 2                   ; ..............................nc  .


.m1loop
  		mov   ebx, eax
  		shr   ebx, 9
  		or    ebx, 0x3f800000							
  		mov   [ebp + syWOsc.tmp], ebx
      cmp   eax, [ebp + syWOsc.brpt] ; ..............................oc  n
      rcl   edx, 1                   ; .............................ocn  .
      and   edx, 7                   ; 00000000000000000000000000000ocn
  		jmp		dword [cs: .m1casetab + 4*edx]

; cases: on entry <cc121> <cc212> <b> <gdf> <g>, on exit: <out> <cc121> <cc212> <b> <gdf> <g>
.m1c21  ; p2->p1 : 2(p-1)/f - 1
      fld   dword [ebp + syWOsc.tmp]   ; <p> <cc121> <cc212> <b> <gdf> <g>
      fld1                             ; <1> <p> <cc121> <cc212> <b> <gdf> <g>
      fsubp st1, st0                   ; <p-1> <cc121> <cc212> <b> <gdf> <g>
      fadd  st0, st0                   ; <2(p-1)> <cc121> <cc212> <b> <gdf> <g>
      fmul  st0, st4                   ; <gdf*2(p-1)> <cc121> <cc212> <b> <gdf> <g>
      fsub  st0, st5                   ; <out> <cc121> <cc212> <b> <gdf> <g>
      jmp   short .m1pl

.m1c12  ; p1->p2 : 2(b-p)/f + 1
      fld   st2                        ; <b> <cc121> <cc212> <b> <gdf> <g>
      fsub  dword [ebp + syWOsc.tmp]   ; <b-p> <cc121> <cc212> <b> <gdf> <g>
      fadd  st0, st0                   ; <2(b-p)> <cc121> <cc212> <b> <gdf> <g>
      fmul  st0, st4                   ; <gdf*2(b-p)> <cc121> <cc212> <b> <gdf> <g>
      fadd  st0, st5                   ; <out> <cc121> <cc212> <b> <gdf> <g>
      jmp   short .m1pl

.m1c121 ; p1->p2->p1 : (2b-4)/f + 1
      fld  st0                          ; <out> <cc121> <cc212> <b> <gdf> <g>
      jmp  short .m1pl

.m1c212 ; p2->p1->p2 : (2b-2)/f - 1
      fld  st1                          ; <out> <cc121> <cc212> <b> <gdf> <g>
      jmp  short .m1pl

.m1c1   ; p1 : 1
      fld   st4                         ; <out> <cc121> <cc212> <b> <gdf> <g>
      jmp  short .m1pl

.m1c2   ; p2 : -1
      fld   st4                         ; <out> <cc121> <cc212> <b> <gdf> <g>
      fchs

.m1pl
      add   eax, esi                 ; ...............................n  c
      rcl   edx, 1                   ; ..............................nc  .
      test  byte [ebp + syWOsc.ring], 1
      jz    .m1noring
  	  fmul	dword [edi + 4*ecx]
      jmp   short .m1store
.m1noring
		  fadd	dword [edi + 4*ecx]
.m1store
		  fstp	dword [edi + 4*ecx]
		  inc		ecx
		jnz   .m1loop
    mov   [ebp + syWOsc.cnt], eax
    fstp  st0 ; <cc212> <b> <gdf> <g>
    fstp  st0 ; <b> <gdf> <g>
    fstp  st0 ; <gdf> <g>
    fstp  st0 ; <g>
    fstp  st0 ; -
    V2PerfLeave V2Perf_OSC
		popad				
    ret


           


.mode2     ; sin
		mov		eax, [ebp + syWOsc.cnt]
		mov   edx, [ebp + syWOsc.freq]
		
		fld   qword [fcsinx7]  ; <cx7>
		fld   qword [fcsinx5]  ; <cx5> <cx7>
		fld   qword [fcsinx3]  ; <cx3> <cx5> <cx7>
		
.m2loop1
			mov   ebx, eax
			add   ebx, 0x40000000
			mov   esi, ebx
			sar		esi, 31
			xor   ebx, esi
			shr   ebx, 8
			add   eax, edx
			or    ebx, 0x3f800000							
			mov   [ebp + syWOsc.tmp], ebx
			fld   dword [ebp + syWOsc.tmp] ; <x> <cx3> <cx5> <cx7>
			
			; scale/move to (-pi/4 .. pi/4)
			fmul  dword [fcpi]             ; <x'> <cx3> <cx5> <cx7>
			fsub  dword [fc1p5pi]          ; <x''> <cx3> <cx5> <cx7>

			; inline fastsin
			fld		st0										   ; <x> <x> <cx3> <cx5> <cx7>
			fmul	st0, st1                 ; <x²> <x> <cx3> <cx5> <cx7>
			fld	  st4                      ; <c> <x²> <x> <cx3> <cx5> <cx7>
			fmul	st0, st1                 ; <cx²> <x²> <x> <cx3> <cx5> <cx7>
			fadd	st0, st4                 ; <b+cx²> <x²> <x> <cx3> <cx5> <cx7>
			fmul  st0, st1                 ; <x²(b+cx²)> <x²> <x> <cx3> <cx5> <cx7>
			fadd  st0, st3                 ; <a+x²(b+cx²)> <x²> <x> <cx3> <cx5> <cx7>
			fmulp st1, st0                 ; <x²(a+x²(b+cx²)> <x> <cx3> <cx5> <cx7>
			fld1                           ; <1> <x²(a+x²(b+cx²)> <x> <cx3> <cx5> <cx7>
			faddp st1, st0                 ; <1+x²(a+x²(b+cx²)> <x> <cx3> <cx5> <cx7>
			fmulp st1, st0                 ; <x(1+x²(a+x²(b+cx²))> <cx3> <cx5> <cx7>
  
			fmul  dword [ebp + syWOsc.gain] ; <gain(y)> <cx3> <cx5> <cx7>
      test  byte [ebp + syWOsc.ring], 1
      jz    .m2noring
			fmul	dword [edi + 4*ecx]       ; <out> <cx3> <cx5> <cx7>
      jmp   short .m2store
.m2noring
			fadd	dword [edi + 4*ecx]       ; <out> <cx3> <cx5> <cx7>
.m2store
			fstp	dword [edi + 4*ecx]       ; <cx3> <cx5> <cx7>
			inc   ecx
		jnz   .m2loop1
    mov   [ebp + syWOsc.cnt], eax
    
    fstp  st0                         ; <cx5> <cx7>
    fstp  st0                         ; <cx7>
    fstp  st0                         ; -
    V2PerfLeave V2Perf_OSC
		popad				
    ret


.mode3     ; noise
      mov   esi,	[ebp + syWOsc.nseed] 
			fld   dword [ebp + syWOsc.nfres]   ; <r>
			fld   dword [ebp + syWOsc.nffrq]   ; <f> <r>
			fld   dword [ebp + syWOsc.nfl]			; <l> <f> <r>
			fld   dword [ebp + syWOsc.nfb]			; <b> <l> <f> <r>
.m3loop1
	    imul	esi, icnoisemul
			add		esi, icnoiseadd
			mov		eax, esi
			shr   eax, 9
			or    eax, 40000000h
			mov		[ebp + syWOsc.tmp], eax
			fld   dword [ebp + syWOsc.tmp]    ; <n> <b> <l> <f> <r>
			fld1															; <1> <n> <b> <l> <f> <r>
			fsub	st1, st0										; <1> <n-1> <b> <l> <f> <r>
			fsub	st1, st0										; <1> <n-2> <b> <l> <f> <r>
			fsubp st1, st0										; <n'> <b> <l> <f> <r>

			fld   st1                         ; <b> <n'> <b> <l> <f> <r>
			fmul  st0, st4                    ; <b*f> <n'> <b> <l> <f> <r>
			fxch  st1													; <n'> <b*f> <b> <l> <f> <r>
			fld   st2                         ; <b> <n'> <b*f> <b> <l> <f> <r>
			fmul  st0, st6                    ; <b*r> <n'> <b*f> <b> <l> <f> <r>
			fsubp st1, st0                    ; <n-(b*r)> <b*f> <b> <l> <f> <r>
			fxch  st1                         ; <b*f> <n-(b*r)> <b> <l> <f> <r>
			faddp st3, st0                    ; <n-(b*r)> <b> <l'> <f> <r>
			fsub  st0, st2                    ; <h> <b> <l'> <f> <r>
			fld   st0                         ; <h> <h> <b> <l'> <f> <r>
			fmul  st0, st4                    ; <h*f> <h> <b> <l'> <f> <r>
			faddp st2, st0                    ; <h> <b'> <l'> <f> <r>
			fld   st2                         ; <l'> <h> <b'> <l'> <f> <r>
			faddp st1, st0                    ; <l+h> <b'> <l'> <f> <r>
			fmul  st0, st4                    ; <r(l+h)> <b'> <l'> <f> <r>
			fadd  st0, st1                    ; <r(l+h)+b> <b'> <l'> <f> <r>

			fmul	dword [ebp + syWOsc.gain]   ; <out> <b'> <l'> <f> <r>
      test  byte [ebp + syWOsc.ring], 1
      jz    .m3noring
			fmul	dword [edi + 4*ecx]
      jmp   .m3store
.m3noring
			fadd	dword [edi + 4*ecx]
.m3store
			fstp	dword [edi + 4*ecx]
			inc		ecx
		jnz   .m3loop1
		fstp	dword [ebp + syWOsc.nfb]			; <l> <f> <r>
		fstp	dword [ebp + syWOsc.nfl]			; <f> <r>
		fstp  st0                           ; <r>
		fstp  st0                           ; <->
    mov   [ebp + syWOsc.nseed], esi
    V2PerfLeave V2Perf_OSC
		popad				
    ret


.mode4     ; fm sin
		mov		eax, [ebp + syWOsc.cnt]
		mov   edx, [ebp + syWOsc.freq]
.m4loop1
			mov   ebx, eax
      fld   dword [edi + 4*ecx]  ; -1 .. 1
      fmul  dword [fcfmmax]
			shr   ebx, 9
			add   eax, edx
			or    ebx, 0x3f800000							
			mov   [ebp + syWOsc.tmp], ebx
			fadd  dword [ebp + syWOsc.tmp]
			fmul  dword [fc2pi]      
			call  fastsinrc
			fmul  dword [ebp + syWOsc.gain]
      test  byte [ebp + syWOsc.ring], 1
      jz    .m4store
			fmul	dword [edi + 4*ecx]
.m4store
			fstp	dword [edi + 4*ecx]
			inc   ecx
		jnz   .m4loop1
    mov   [ebp + syWOsc.cnt], eax
    V2PerfLeave V2Perf_OSC
		popad				
    ret

; CHAOS

.auxa			; copy 
		lea   esi,[auxabuf]
.auxaloop
			fld		dword [esi+0]
			fadd	dword [esi+4]
			add		esi,8
			fmul  dword [ebp + syWOsc.gain]
			fmul	dword [fcgain]
      test  byte [ebp + syWOsc.ring], 1
      jz    .auxastore
			fmul	dword [edi + 4*ecx]
.auxastore
			fstp	dword [edi + 4*ecx]
			inc   ecx
		jnz   .auxaloop
    V2PerfLeave V2Perf_OSC
		popad
		ret



.auxb			; copy 
		lea   esi,[auxbbuf]
		jmp .auxaloop		

;#####################################################################################
;  Envelope Generator
;#####################################################################################


global _ENV_
_ENV_

struc syVEnv
  .ar		resd 1   
	.dr   resd 1
	.sl   resd 1
	.sr   resd 1
	.rr   resd 1
	.vol  resd 1
	.size
endstruc

struc syWEnv
	.out    resd 1
  .state  resd 1  ; int state - 0: off, 1: attack, 2: decay, 3: sustain, 4: release 
	.val    resd 1  ; float outval (0.0-128.0)
  .atd		resd 1  ; float attack delta (added every frame in phase 1, transition -> 2 at 128.0)
	.dcf		resd 1  ; float decay factor (mul'ed every frame in phase 2, transision -> 3 at .sul)
	.sul		resd 1  ; float sustain level (defines phase 2->3 transition point)
	.suf		resd 1  ; float sustain factor (mul'ed every frame in phase 3, transition -> 4 at gate off or ->0 at 0.0)
	.ref    resd 1  ; float release (mul'ed every frame in phase 4, transition ->0 at 0.0)
	.gain   resd 1  ; float gain (0.1 .. 1.0)
	.size
endstruc

; init
; ebp: workspace
syEnvInit:
	   pushad
		 xor eax, eax
		 mov [ebp + syWEnv.state], eax  ; reset state to "off"
		 popad
		 ret

; set
; esi: values
; epb: workspace
syEnvSet:
		pushad

		; ar: 2^7 (128) bis 2^-4 (0.03, ca. 10secs bei 344frames/sec)
		fld		dword [esi + syVEnv.ar]		; 0..127
		fmul  dword [fcattackmul]				; 0..-12
		fadd  dword [fcattackadd]				; 7..-5
		call  pow2
		fstp  dword [ebp + syWEnv.atd]

		; dcf: 0 (5msecs dank volramping) bis fast 1 (durchgehend)
		fld		dword [esi + syVEnv.dr]		; 0..127
		fmul  dword [fci128]            ; 0..~1
		fld1                            ; 1  0..~1
		fsubrp st1, st0                 ; 1..~0
		call  calcfreq2                
		fld1                    
		fsubrp st1, st0
		fstp  dword [ebp + syWEnv.dcf]  ; 0..~1

		; sul: 0..127 ist schon ok
		fld		dword [esi + syVEnv.sl]		; 0..127
		fstp  dword [ebp + syWEnv.sul]  ; 0..127

		; suf: 1/128 (15msecs bis weg) bis 128 (15msecs bis voll da)
		fld		dword [esi + syVEnv.sr]		; 0..127
		fsub  dword [fc64]              ; -64 .. 63
		fmul  dword [fcsusmul]          ; -7 .. ~7
		call  pow2
		fstp  dword [ebp + syWEnv.suf]  ; 1/128 .. ~128

		; ref: 0 (5msecs dank volramping) bis fast 1 (durchgehend)
		fld		dword [esi + syVEnv.rr]		; 0..127
		fmul  dword [fci128]
		fld1
		fsubrp st1, st0
		call  calcfreq2
		fld1
		fsubrp st1, st0
		fstp  dword [ebp + syWEnv.ref]  ; 0..~1
		
		fld   dword [esi + syVEnv.vol]
		fmul  dword [fci128]
		fstp  dword [ebp + syWEnv.gain]

		popad 
		ret


; tick
; epb: workspace
; ATTENTION: eax: gate

syETTab		dd syEnvTick.state_off, syEnvTick.state_atk, syEnvTick.state_dec,
				  dd syEnvTick.state_sus, syEnvTick.state_rel

syEnvTick:
		pushad
		mov		ebx, [ebp + syWEnv.state]
		call  [syETTab + 4*ebx]
		fld   dword [ebp + syWEnv.val]
		fmul  dword [ebp + syWEnv.gain]
		fstp  dword [ebp + syWEnv.out]
		popad 
		ret

.state_off  ; envelope off
    or    eax, eax  ; gate on -> attack
		jz		.s0ok
		inc		byte [ebp + syWEnv.state]
		jmp   .state_atk
.s0ok
		fldz
		fstp  dword [ebp + syWEnv.val]
		ret

.state_atk  ; attack
    or    eax, eax  ; gate off -> release
		jnz		.s1ok
		mov		byte [ebp + syWEnv.state], 4
		jmp   .state_rel
.s1ok 
		fld   dword [ebp + syWEnv.val]
		fadd  dword [ebp + syWEnv.atd]
		fstp	dword [ebp + syWEnv.val]
		mov   ecx,  43000000h           ; 128.0
		cmp		ecx,	[ebp + syWEnv.val]
		ja    .s1end                    ; val above -> decay
		mov   [ebp + syWEnv.val], ecx
		inc   byte [ebp + syWEnv.state]
.s1end
		ret

.state_dec
    or    eax, eax  ; gate off -> release
		jnz		.s2ok
		mov		byte [ebp + syWEnv.state], 4
		jmp   .state_rel
.s2ok 
		fld   dword [ebp + syWEnv.val]
		fmul  dword [ebp + syWEnv.dcf]
		fstp	dword [ebp + syWEnv.val]
		mov   ecx,  [ebp + syWEnv.sul] ; sustain level
		cmp		ecx,	[ebp + syWEnv.val]
		jb    .s4checkrunout            ; val below -> sustain
		mov   [ebp + syWEnv.val], ecx
		inc   byte [ebp + syWEnv.state]
		ret


.state_sus
    or    eax, eax  ; gate off -> release
		jnz		.s3ok
		inc		byte [ebp + syWEnv.state]
		jmp   .state_rel
.s3ok 
		fld   dword [ebp + syWEnv.val]
		fmul  dword [ebp + syWEnv.suf]
		fstp	dword [ebp + syWEnv.val]
		mov   ecx,  LOWEST
		cmp		ecx,	[ebp + syWEnv.val]
		jb    .s3not2low               ; val below -> off
		xor   ecx, ecx
		mov   [ebp + syWEnv.val], ecx
		mov   [ebp + syWEnv.state], ecx
		ret
.s3not2low
		mov   ecx,  43000000h           ; 128.0
		cmp		ecx,	[ebp + syWEnv.val]
		ja    .s3end                    ; val above -> decay
		mov   [ebp + syWEnv.val], ecx
.s3end
		ret


.state_rel
    or    eax, eax  ; gate off -> release
		jz		.s4ok
		mov		byte [ebp + syWEnv.state], 1
		jmp   .state_atk
.s4ok 
		fld   dword [ebp + syWEnv.val]
		fmul  dword [ebp + syWEnv.ref]
		fstp	dword [ebp + syWEnv.val]
.s4checkrunout
		mov   ecx,  LOWEST
		cmp		ecx,	[ebp + syWEnv.val]
		jb    .s4end
		xor   ecx, ecx
		mov   [ebp + syWEnv.val], ecx
		mov   [ebp + syWEnv.state], ecx
.s4end
		ret



;#####################################################################################
;  Filter
;#####################################################################################

global _VCF_
_VCF_


struc syVFlt
	.mode   resd 1
	.cutoff resd 1
	.reso   resd 1
	.size
endstruc

struc syWFlt
  .mode   resd 1  ; int: 0 - bypass, 1 - low, 2 - band, 3 - high, 4 - notch, 5 - all
	.cfreq  resd 1  ; float: frq (0-1)
	.res    resd 1  ; float: res (0-1)
	.l      resd 1
	.b      resd 1
  .step   resd 1
	.size
endstruc

syFltInit:
	pushad
	xor eax, eax
	mov [ebp + syWFlt.l], eax
	mov [ebp + syWFlt.b], eax
  mov  al, 4
  mov [ebp + syWFlt.step], eax
	popad
	ret


syFltSet:
	pushad
	fld			dword [esi + syVFlt.mode]
	fistp		dword [ebp + syWFlt.mode]

	fld			dword [esi + syVFlt.cutoff]
	fmul		dword [fci128]
	call    calcfreq


  fld			dword [esi + syVFlt.reso]  ; <r> <fr> <fr>
	fmul		dword [fci128]
	fld1															 ; <1> <r> <fr>
  fsubrp  st1, st0                   ; <1-res> <fr>
	fstp		dword [ebp + syWFlt.res]   ; <fr>

	fmul    dword [SRfclinfreq]
	fstp	  dword [ebp + syWFlt.cfreq] ; -

	popad
	ret

syFRTab	dd syFltRender.mode0, syFltRender.mode1, syFltRender.mode2
				dd syFltRender.mode3, syFltRender.mode4, syFltRender.mode5
				dd syFltRender.mode0, syFltRender.mode0


; ebp: workspace
; ecx: count
; esi: source
; edi: dest
syFltRender:
	pushad
	V2PerfEnter V2Perf_VCF
	fld   dword [ebp + syWFlt.res]	; <r>
	fld   dword [ebp + syWFlt.cfreq]; <f> <r>
	fld   dword [ebp + syWFlt.l]		; <l> <f> <r>
	fld   dword [ebp + syWFlt.b]		; <b> <l> <f> <r>
	movzx	eax, byte [ebp + syWFlt.mode]
	and   al, 7
	call  [syFRTab + 4*eax]
  fstp dword [ebp + syWFlt.b] ; <l''> <f> <r>
  fstp dword [ebp + syWFlt.l] ; <f> <r>
	fstp st0
	fstp st0
	V2PerfLeave V2Perf_VCF
	popad
	ret

.process: ; <b> <l> <f> <r>
	  fld			dword [esi]									; <in> <b> <l> <f> <r>
%if FIXDENORMALS
    fadd    dword [dcoffset]
%endif    	  
		fld			st1                         ; <b> <in> <b> <l> <f> <r>
		fld			st2                         ; <b> <b> <in> <b> <l> <f> <r>
		fmul		st0, st5                    ; <b*f> <b> <in> <b> <l> <f> <r>
		fxch		st1                         ; <b> <b*f> <in> <b> <l> <f> <r>
		fmul		st0, st6                    ; <b*r> <b*f> <in> <b> <l> <f> <r>
		fxch		st1                         ; <b*f> <b*r> <in> <b> <l> <f> <r>
		add     esi, [ebp + syWFlt.step]
		faddp		st4, st0										; <b*r> <in> <b> <l'> <f> <r>
		fsubr   st0, st1                    ; <in-b*r> <in> <b> <l'> <f> <r>
		fsub    st0, st3                    ; <h> <in> <b> <l'> <f> <r>
		fmul    st0, st4                    ; <f*h> <in> <b> <l'> <f> <r>
		faddp   st2, st0                    ; <in> <b'> <l'> <f> <r>
		fld     st1                         ; <b'> <in> <b'> <l'> <f> <r>
		fld     st2                         ; <b'> <b'> <in> <b'> <l'> <f> <r>
		fmul    st0, st5                    ; <b'*f> <b'> <in> <b'> <l'> <f> <r>
		fxch		st1													; <b'> <b'*f> <in> <b'> <l'> <f> <r>
		fmul    st0, st6                    ; <b'*r> <b'*f> <in> <b'> <l'> <f> <r>
		fxch    st1                         ; <b'*f> <b'*r> <in> <b'> <l'> <f> <r>
		faddp   st4, st0                    ; <b'*r> <in> <b'> <l''> <f> <r>
		fsubp   st1, st0                    ; <in-b'*r> <b'> <l''> <f> <r>
		fsub    st0, st2                    ; <h> <b'> <l''> <f> <r>
		fld     st0                         ; <h> <h> <b'> <l''> <f> <r>
		fmul    st0, st4                    ; <h*f> <h> <b'> <l''> <f> <r>
		faddp   st2, st0                    ; <h> <b''> <l''> <f> <r>
		ret


.mode0: ; bypass
  cmp		esi, edi
	je    .m0end
	rep   movsd
.m0end
	ret

.mode1: ; low
    call	.process			; <h> <b''> <l''> <f> <r>
		fstp	st0						; <b''> <l''> <f> <r>
		fxch  st1						; <l''> <b''> <f> <r>
		fst   dword [edi]   ; -> l'' stored
		fxch  st1						; <b''> <l''> <f> <r>
    add   edi, [ebp + syWFlt.step]
	dec ecx
	jnz .mode1
	ret

.mode2: ; band
    call	.process			; <h> <b''> <l''> <f> <r>
		fstp	st0						; <b''> <l''> <f> <r>
		fst   dword [edi]   ; -> b'' stored
    add   edi, [ebp + syWFlt.step]
	dec ecx
	jnz	.mode2
	ret

.mode3: ; high
    call	.process			; <h> <b''> <l''> <f> <r>
		fstp  dword [edi]		; <b''> <l''> <f> <r> -> h stored
    add   edi, [ebp + syWFlt.step]
	dec ecx
	jnz	.mode3
	ret
		
.mode4: ; notch
    call	.process			; <h> <b''> <l''> <f> <r>
		fadd  st2           ; <h+l> <b''> <l''> <f> <r>
		fstp  dword [edi]		; <b''> <l''> <f> <r> -> h+l'' stored
    add   edi, [ebp + syWFlt.step]
	dec ecx
	jnz	.mode4
	ret

.mode5: ; allpass
    call	.process			; <h> <b''> <l''> <f> <r>
		fadd  st1           ; <h+b> <b''> <l''> <f> <r>
		fadd  st2           ; <h+b+l> <b''> <l''> <f> <r>
		fstp  dword [edi]		; <b''> <l''> <f> <r> -> h+b''+l'' stored
    add   edi, [ebp + syWFlt.step]
	dec ecx
	jnz	.mode5
	ret



;#####################################################################################
; Low Frequency Oscillator
;#####################################################################################

global _LFO_
_LFO_

struc syVLFO
  .mode		resd 1  ; 0: saw, 1: tri, 2: pulse, 3: sin, 4: s&h
  .sync		resd 1  ; 0: free  1: in sync with keyon
  .egmode	resd 1  ; 0: continuous  1: one-shot (EG mode)
  .rate		resd 1  ; rate (0Hz .. ~43hz)
  .phase	resd 1  ; start phase shift
  .pol      resd 1  ; polarity: + , . , +/-
  .amp		resd 1  ; amplification (0 .. 1)
  .size
endstruc

struc syWLFO
	.out		resd 1  ; float: out
	.mode   resd 1  ; int: mode
	.fs     resd 1  ; int: sync flag
	.feg    resd 1  ; int: eg flag
	.freq   resd 1  ; int: freq
	.cntr   resd 1  ;	int: counter
	.cph    resd 1  ; int: counter sync phase
	.gain   resd 1  ; float: output gain
	.dc     resd 1  ; float: output DC
	.nseed  resd 1  ; int: random seed
	.last   resd 1  ; int: last counter value (for s&h transition)
  .size
endstruc


syLFOInit
  pushad
	xor			eax, eax
	mov			[ebp + syWLFO.cntr], eax
	mov			[ebp + syWLFO.last], eax
	rdtsc
	mov			[ebp + syWLFO.nseed], eax
	popad
	ret


syLFOSet
  pushad
	
	fld		dword [esi + syVLFO.mode]
	fistp   dword [ebp + syWLFO.mode]
	fld     dword [esi + syVLFO.sync]
	fistp   dword [ebp + syWLFO.fs]
	fld     dword [esi + syVLFO.egmode]
	fistp   dword [ebp + syWLFO.feg]

	fld     dword [esi + syVLFO.rate]
	fmul    dword [fci128]
	call    calcfreq
	fmul    dword [fc32bit]
	fmul    dword [fci2]
	fistp   dword [ebp + syWLFO.freq]

	fld     dword [esi + syVLFO.phase]
	fmul    dword [fci128]
	fmul    dword [fc32bit]
	fistp   dword [ebp + syWLFO.cph]
	shl     dword [ebp + syWLFO.cph],1

	fld     dword [esi + syVLFO.amp]
	fld     dword [esi + syVLFO.pol]
	fistp   dword [temp]
	mov     eax,  [temp]
	fld     st0
	fchs
	fmul    dword [fci2]
	cmp     al, 2       ; +/- polarity?
	jz      .isp2
	fsub    st0, st0	
.isp2
	fstp    dword [ebp + syWLFO.dc]
	cmp     al, 1       ; +/- polarity?
	jnz     .isntp1
	fchs
.isntp1
	fstp    dword [ebp + syWLFO.gain]

	popad
	ret


syLFOKeyOn
  pushad
	mov eax, [ebp + syWLFO.fs]
	or  eax, eax
	jz  .end
	mov eax, [ebp + syWLFO.cph]
	mov [ebp + syWLFO.cntr], eax
	xor eax, eax
	not eax
	mov [ebp + syWLFO.last], eax
.end
	popad
	ret


syLTTab		dd syLFOTick.mode0, syLFOTick.mode1, syLFOTick.mode2, syLFOTick.mode3
				  dd syLFOTick.mode4, syLFOTick.mode0, syLFOTick.mode0, syLFOTick.mode0

syLFOTick
  pushad
	mov			eax, [ebp + syWLFO.cntr]
	mov			edx, [ebp + syWLFO.mode]
	and     dl, 7
	call    [syLTTab + 4*edx]
	fmul    dword [ebp + syWLFO.gain]
	fadd    dword [ebp + syWLFO.dc]
	fstp    dword [ebp + syWLFO.out]
	mov			eax, [ebp + syWLFO.cntr]
	add			eax, [ebp + syWLFO.freq]
	jnc     .isok
	mov     edx, [ebp + syWLFO.feg]
	or      edx, edx
	jz      .isok
	xor     eax, eax
	not     eax
.isok
	mov			[ebp + syWLFO.cntr], eax
	popad
	ret

.mode0 ; saw
  shr			eax, 9
	or      eax, 3f800000h
	mov     [temp], eax
	fld     dword [temp] ; <1..2>
	fld1                 ; <1> <1..2>
	fsubp   st1, st0     ; <0..1>
	ret

.mode1 ; tri
  shl			eax, 1
	sbb     ebx, ebx
	xor     eax, ebx
	jmp     .mode0

.mode2 ; pulse
	shl     eax, 1
	sbb     eax, eax
  jmp			.mode0


.mode3 ; sin
  call    .mode0
	fmul    dword [fc2pi] ; <0..2pi>
	call    fastsinrc     ; <-1..1>
	fmul    dword [fci2]  ; <-0.5..0.5>
	fadd    dword [fci2]  ; <0..1>
	ret


.mode4 ; s&h
	cmp   eax, [ebp + syWLFO.last]
	mov		[ebp + syWLFO.last], eax
  jae   .nonew
	mov   eax, [ebp + syWLFO.nseed]
  imul	eax, icnoisemul
	add		eax, icnoiseadd
	mov		[ebp + syWLFO.nseed], eax
.nonew
	mov   eax, [ebp + syWLFO.nseed]
	jmp   .mode0




;#####################################################################################
; INDUSTRIALGELÖT UND UNKAPUTTBARE ORGELN
;  Das Verzerr- und Verüb-Modul! 
;#####################################################################################


; mode1:  overdrive ...  input gain, output gain, offset
; mode2:  clip...        input gain, output gain, offset
; mode3:  bitcrusher...  input gain, crush, xor
; mode4:  decimator...   -,  resamplingfreq, -
; mode5..9:  filters     -,  cutoff, reso

global _DIST_
_DIST_

struc syVDist
	.mode			resd 1			; 0: off, 1: overdrive, 2: clip, 3: bitcrusher, 4: decimator
                        ; modes 4 to 8: filters (see syVFlt)
	.ingain   resd 1      ; -12dB ... 36dB
	.param1   resd 1      ; outgain/crush/outfreq
	.param2   resd 1      ; offset/offset/xor/jitter
	.size
endstruc

struc syWDist
	.mode			resd 1
	.gain1    resd 1      ; float: input gain for all fx
	.gain2    resd 1      ; float: output gain for od/clip
	.offs			resd 1      ; float: offs for od/clip
	.crush1   resd 1      ; float: crush factor^-1
	.crush2   resd 1      ; int:	 crush factor^1
	.crxor    resd 1      ; int:   xor value for crush
	.dcount   resd 1      ; int:   decimator counter
	.dfreq    resd 1      ; int:   decimator frequency
	.dvall	  resd 1			; float: last decimator value (mono/left)
	.dvalr	  resd 1			; float: last decimator value (right)
	.dlp1c    resd 1			; float: decimator pre-filter coefficient
	.dlp1bl   resd 1			; float: decimator pre-filter buffer (mono/left)
	.dlp1br   resd 1			; float: decimator pre-filter buffer (right)
	.dlp2c    resd 1			; float: decimator post-filter coefficient
	.dlp2bl   resd 1			; float: decimator post-filter buffer (mono/left)
	.dlp2br   resd 1			; float: decimator post-filter buffer (right)
	.fw1      resb syWFlt.size  ; left/mono filter workspace
	.fw2      resb syWFlt.size  ; right filter workspace
	.size
endstruc


syDistInit:
		pushad
		xor			eax, eax
		mov			[ebp + syWDist.dcount], eax
		mov			[ebp + syWDist.dvall], eax
		mov			[ebp + syWDist.dvalr], eax
		mov			[ebp + syWDist.dlp1bl], eax
		mov			[ebp + syWDist.dlp1br], eax
		mov			[ebp + syWDist.dlp2bl], eax
		mov			[ebp + syWDist.dlp2br], eax
    lea     ebp, [ebp + syWDist.fw1]
    call    syFltInit
    lea     ebp, [ebp + syWDist.fw2 - syWDist.fw1]
    call    syFltInit
		popad
		ret

section .data

syDSTab			dd syDistSet.mode0, syDistSet.mode1, syDistSet.mode2, syDistSet.mode3, syDistSet.mode4
						dd syDistSet.modeF, syDistSet.modeF, syDistSet.modeF, syDistSet.modeF, syDistSet.modeF
						dd syDistSet.mode0, syDistSet.mode0, syDistSet.mode0, syDistSet.mode0, syDistSet.mode0
						dd syDistSet.mode0

section .text

syDistSet:
		pushad
		fld			dword [esi + syVDist.mode]
		fistp   dword [ebp + syWDist.mode]

		fld     dword [esi + syVDist.ingain]
		fsub    dword [fc32]
		fmul    dword [fci16]
		call    pow2
		fstp    dword [ebp + syWDist.gain1]

		fld     dword [esi + syVDist.param1]
		mov     eax, [ebp + syWDist.mode]
		and     al, 15
		call    [syDSTab + 4*eax]
		popad
		ret

.mode0
		fstp st0
		ret

.mode1 ; overdrive
 		fmul    dword [fci128]
		fld			dword [ebp + syWDist.gain1]
		fld1
		fpatan
		fdivp   st1, st0
		jmp     .mode2b

.mode2 ; clip
 		fmul    dword [fci128]
.mode2b
		fstp    dword [ebp + syWDist.gain2]
		fld     dword [esi + syVDist.param2]
		fsub    dword [fc64]
		fmul    dword [fci128]		
		fadd    st0, st0
		fmul    dword [ebp + syWDist.gain1]
		fstp    dword [ebp + syWDist.offs]
		ret

.mode3 ; bitcrusher
		fmul    dword [fc256]                  ; 0 .. 32xxx
		fld1 
		faddp   st1, st0                       ; 1 .. 32xxx
		fld     dword [fc32768]                ; 32768 x
		fxch    st1														 ; x 32768
		fdiv    st1, st0                       ; x 32768/x
		fistp   dword [ebp + syWDist.crush2]
		fmul    dword [ebp + syWDist.gain1]
		fstp    dword [ebp + syWDist.crush1]
		fld     dword [esi + syVDist.param2]
		fistp   dword [temp]
		mov     eax, [temp]
		shl     eax, 9
		mov     [ebp + syWDist.crxor], eax
		ret

.mode4 ; decimator
		fmul    dword [fci128]
		call    calcfreq
		fmul    dword [fc32bit]
		fistp   dword [ebp + syWDist.dfreq]
		shl     dword [ebp + syWDist.dfreq], 1
		fld     dword [esi + syVDist.ingain]
		fmul    dword [fci127]
		fmul    st0, st0
		fstp    dword [ebp + syWDist.dlp1c]
		fld     dword [esi + syVDist.param2]
		fmul    dword [fci127]
		fmul    st0, st0
		fstp    dword [ebp + syWDist.dlp2c]
    ret

.modeF ; filters
    fstp    dword [temp + syVFlt.cutoff]    
    fld     dword [esi + syVDist.param2]
    fstp    dword [temp + syVFlt.reso]
    mov     eax, [ebp + syWDist.mode]
    lea     eax, [eax-4]
    mov     [temp + syVFlt.mode], eax
    fild    dword [temp + syVFlt.mode]
    fstp    dword [temp + syVFlt.mode]
    lea     esi, [temp]
    lea     ebp, [ebp + syWDist.fw1]
    call    syFltSet
    lea     ebp, [ebp + syWDist.fw2 - syWDist.fw1]
    jmp     syFltSet

section .data

syDRMTab		dd syDistRenderMono.mode0, syDistRenderMono.mode1, syDistRenderMono.mode2, syDistRenderMono.mode3
						dd syDistRenderMono.mode4, syDistRenderMono.modeF, syDistRenderMono.modeF, syDistRenderMono.modeF
            dd syDistRenderMono.modeF, syDistRenderMono.modeF, syDistRenderMono.mode0, syDistRenderMono.mode0
            dd syDistRenderMono.mode0, syDistRenderMono.mode0, syDistRenderMono.mode0, syDistRenderMono.mode0

section .text

; esi : sourceptr
; edi : destptr
; ecx : size
syDistRenderMono:
		pushad
	  V2PerfEnter V2Perf_DIST
		mov			edx, [ebp + syWDist.mode]
		call    [syDRMTab + 4*edx]
	  V2PerfLeave V2Perf_DIST
		popad
		ret

.mode0  ; bypass
		cmp			esi, edi
		je			.m0end
		rep     movsd
.m0end
		ret

.mode1  ; overdrive
			fld1							; <1>
			fchs							; <-1>
			xor   ebx, ebx
			mov   bl, 8
.m1loop
			fld		dword [esi]
			lea   esi, [esi+4]
			fmul  dword [ebp + syWDist.gain1]
			fadd  dword [ebp + syWDist.offs]
			fst   dword [temp]
			mov   ax,   [temp+2]

			call  fastatan
			
			fmul  dword [ebp + syWDist.gain2]
			fstp  dword [edi]
			lea   edi, [edi+4]
		dec ecx
		jnz .m1loop
	  fstp  st0          ; -
		ret

.mode2 ; clip
			fld		dword [esi]
			lea   esi, [esi+4]
			fmul  dword [ebp + syWDist.gain1]
			fadd  dword [ebp + syWDist.offs]

      fld1  
      fcomi		st0, st1
      fxch		st1
      fcmovb  st0, st1
      fxch    st1
      fchs    
      fcomi   st0, st1
      fcmovb  st0, st1
      fxch    st1
      fstp    st0
			
			fmul  dword [ebp + syWDist.gain2]
		  fstp  dword [edi]
			lea   edi, [edi+4]
		dec ecx
		jnz .mode2
		ret

.mode3 ; bitcrusher
    mov   ebx, 7fffh
    mov   edx, -7fffh
.m3loop      
      fld		dword [esi]
			lea		esi,	[esi+4]
			fmul	dword [ebp + syWDist.crush1]
			fistp dword [temp]
			mov		eax,	[temp]
			imul  eax,  [ebp + syWDist.crush2]
			cmp			ebx, eax
			cmovle  eax, ebx
			cmp			edx, eax
			cmovge  eax, edx
      xor   eax, [ebp + syWDist.crxor]
			mov   [temp],	eax
			fild  dword [temp]
			fmul  dword [fci32768]
			fstp  dword [edi]
			lea   edi, [edi+4]
			dec   ecx
		jnz .m3loop
		ret


.mode4 ; decimator
  mov eax, [ebp + syWDist.dvall]
  mov edx, [ebp + syWDist.dfreq]
  mov ebx, [ebp + syWDist.dcount]
.m4loop
		add		ebx, edx
		cmovc eax, [esi]
		add		esi, byte 4
		stosd
		dec ecx
  jnz .m4loop
  mov [ebp + syWDist.dcount], ebx
  mov [ebp + syWDist.dvall], eax
  ret

			      

.modeF ; filters
  lea   ebp, [ebp + syWDist.fw1]
  jmp   syFltRender

section .data					

syDRSTab		dd syDistRenderMono.mode0, syDistRenderMono.mode1, syDistRenderMono.mode2, syDistRenderMono.mode3
						dd syDistRenderStereo.mode4, syDistRenderStereo.modeF, syDistRenderStereo.modeF, syDistRenderStereo.modeF
            dd syDistRenderStereo.modeF, syDistRenderStereo.modeF, syDistRenderMono.mode0, syDistRenderMono.mode0
            dd syDistRenderMono.mode0, syDistRenderMono.mode0, syDistRenderMono.mode0, syDistRenderMono.mode0
		
section .text		
		
syDistRenderStereo:
	pushad
	V2PerfEnter V2Perf_DIST
	shl  ecx, 1
	mov			edx, [ebp + syWDist.mode]
	call    [syDRSTab + 4*edx]
	V2PerfLeave V2Perf_DIST
	popad
	ret

.mode4 ; decimator
	shr ecx, 1
	mov eax, [ebp + syWDist.dvall]
	mov edx, [ebp + syWDist.dvalr]
	mov ebx, [ebp + syWDist.dcount]
.m4loop
		add   ebx, [ebp + syWDist.dfreq]
		cmovc eax, [esi]
		cmovc edx, [esi + 4]
		add   esi, byte 8
		stosd 
		xchg  eax, edx
		stosd
		xchg  eax, edx
		dec   ecx
	jnz .m4loop
  mov [ebp + syWDist.dcount], ebx
  mov [ebp + syWDist.dvall],  eax
  mov [ebp + syWDist.dvalr],  edx
	ret





.modeF ; filters
    shr   ecx, 1
    xor   eax, eax
    mov   al, 8
    mov   [ebp + syWDist.fw1 + syWFlt.step], eax
    mov   [ebp + syWDist.fw2 + syWFlt.step], eax
    lea   ebp, [ebp + syWDist.fw1]
    call  syFltRender
    lea   ebp, [ebp + syWDist.fw2 - syWDist.fw1]
    lea   esi, [esi + 4]
    lea   edi, [edi + 4]
    jmp   syFltRender


;#####################################################################################
; V2 - Voice
;#####################################################################################


global _V2V_
_V2V_

struc syVV2
  .panning  resd 1              ; panning
  .transp   resd 1              ; transpose
  .osc1			resb syVOsc.size    ; oszi 1
  .osc2			resb syVOsc.size    ; oszi 2
  .osc3			resb syVOsc.size    ; oszi 3
	.vcf1     resb syVFlt.size	  ; filter 1
	.vcf2     resb syVFlt.size	  ; filter 2
	.routing  resd 1              ; 0: single  1: serial  2: parallel
  .fltbal   resd 1              ; parallel filter balance
	.dist     resb syVDist.size   ; distorter
	.aenv     resb syVEnv.size    ; amplitude env
	.env2			resb syVEnv.size    ; EG 2
	.lfo1     resb syVLFO.size    ; LFO 1
	.lfo2     resb syVLFO.size    ; LFO 2
	.oscsync  resd 1              ; osc keysync flag
	.size
endstruc


struc syWV2
	.note			resd 1
	.velo			resd 1
	.gate     resd 1

	.curvol   resd 1
	.volramp  resd 1

  .xpose    resd 1
	.fmode    resd 1
	.lvol     resd 1
	.rvol     resd 1
  .f1gain   resd 1
  .f2gain   resd 1

	.oks      resd 1

	.osc1     resb syWOsc.size
	.osc2     resb syWOsc.size
	.osc3     resb syWOsc.size
	.vcf1     resb syWFlt.size	  ; filter 1
	.vcf2     resb syWFlt.size	  ; filter 2
	.aenv     resb syWEnv.size
	.env2			resb syWEnv.size    
	.lfo1     resb syWLFO.size    ; LFO 1
	.lfo2     resb syWLFO.size    ; LFO 2
	.dist     resb syWDist.size   ; distorter

	.size
endstruc


; ebp: workspace
syV2Init:
	pushad
	lea ebp, [ebp + syWV2.osc1 - 0]
	call syOscInit
	lea ebp, [ebp + syWV2.osc2 - syWV2.osc1]
	call syOscInit
	lea ebp, [ebp + syWV2.osc3 - syWV2.osc2]
	call syOscInit
	lea ebp, [ebp + syWV2.aenv - syWV2.osc3]
	call syEnvInit
	lea ebp, [ebp + syWV2.env2 - syWV2.aenv]
	call syEnvInit
	lea ebp, [ebp + syWV2.vcf1 - syWV2.env2]
	call syFltInit
	lea ebp, [ebp + syWV2.vcf2 - syWV2.vcf1]
	call syFltInit
	lea ebp, [ebp + syWV2.lfo1 - syWV2.vcf2]
	call syLFOInit
	lea ebp, [ebp + syWV2.lfo2 - syWV2.lfo1]
	call syLFOInit
	lea ebp, [ebp + syWV2.dist - syWV2.lfo2]
	call syDistInit
	popad
	ret


; tick
; ebp: workspace
syV2Tick:
	pushad


	; 1. EGs
	mov		eax, [ebp + syWV2.gate]
	lea		ebp, [ebp + syWV2.aenv - 0]
	call  syEnvTick
	lea		ebp, [ebp + syWV2.env2 - syWV2.aenv]
	call  syEnvTick

	; 2. LFOs
	lea		ebp, [ebp + syWV2.lfo1 - syWV2.env2]
	call  syLFOTick
	lea		ebp, [ebp + syWV2.lfo2 - syWV2.lfo1]
	call  syLFOTick

	lea		ebp, [ebp + 0 - syWV2.lfo2]

	; 3. Volume Ramping
	fld	 dword [ebp + syWV2.aenv + syWEnv.out]
	fmul dword [fci128]
	fsub dword [ebp + syWV2.curvol]
	fmul dword [fciframe]
	fstp dword [ebp + syWV2.volramp]

	popad
	ret


; render
; edi: dest buf
; ecx: # of samples
; ebp: workspace
syV2Render:
	pushad

	; clear buffer
	push ecx
	mov  edi, vcebuf
	xor  eax, eax
	rep  stosd
	pop  ecx

	mov  edi, vcebuf
	lea  ebp, [ebp - 0 + syWV2.osc1]
	call syOscRender
	lea  ebp, [ebp - syWV2.osc1 + syWV2.osc2]
	call syOscRender
	lea  ebp, [ebp - syWV2.osc2 + syWV2.osc3]
	call syOscRender
	lea  ebp, [ebp - syWV2.osc3 + 0]

	; Filter + Routing
	; wenn parallel -> erst filter 2 in buf2 rendern
	mov  edx, [ebp + syWV2.fmode]
	cmp  dl, 2   ; parallel?
	jne  .nopar1
	lea  ebp, [ebp - 0 + syWV2.vcf2]
	mov  esi, vcebuf
	mov  edi, vcebuf2
	call syFltRender
	lea  ebp, [ebp - syWV2.vcf2 + 0]
.nopar1
	; dann auf jeden fall filter 1 rendern
	lea  ebp, [ebp - 0 + syWV2.vcf1]
	mov  esi, vcebuf
	mov  edi, vcebuf
	call syFltRender
	lea  ebp, [ebp - syWV2.vcf1 + 0]
	; dann fallunterscheidung...
	or   dl, dl   ; single?
	jz	 .fltend
	cmp  dl, 2    ; parallel?
	jnz  .nopar2
	push ecx      ; ja -> buf2 auf buf adden
	mov  esi, vcebuf2
	mov  edi, vcebuf
  fld  dword [ebp + syWV2.f1gain]   ; <g1>
  fld  dword [ebp + syWV2.f2gain]   ; <g2> <g1>
.parloop
    fld		dword [esi]               ; <v2> <g2> <g1>
    fmul  st0, st1                  ; <v2'> <g2> <g1>
		add		esi, byte 4    
		fld 	dword [edi]               ; <v1> <v2'> <g2> <g1>
		fmul	st0, st3                  ; <v1'> <v2'> <g2> <g1>
    faddp st1, st0                  ; <out> <g2> <g1>
		fstp  dword [edi]               ; <g2> <g1>
		add   edi, byte 4
	  dec ecx
	jnz .parloop
  fstp st0                          ; <g1>
  fstp st0                          ; -
	pop ecx
	jmp .fltend
.nopar2  
  ; ... also seriell ... filter 2 drüberrechnen
	lea  ebp, [ebp - 0 + syWV2.vcf2]
	mov  esi, vcebuf
	mov  edi, vcebuf
	call syFltRender
	lea  ebp, [ebp - syWV2.vcf2 + 0]
 
.fltend

	; distortion
	mov  esi, vcebuf
	mov  edi, vcebuf
	lea  ebp, [ebp - 0 + syWV2.dist]
	call syDistRenderMono
	lea  ebp, [ebp - syWV2.dist + 0]

	; vcebuf (mono) nach chanbuf(stereo) kopieren
	mov	 edi, chanbuf
	mov  esi, vcebuf
	fld  dword [ebp + syWV2.curvol] ; cv
.copyloop1
		fld		dword [esi]		; out cv
		fmul  st1						; out' cv
		fxch  st1						; cv out'
		fadd  dword [ebp + syWV2.volramp] ; cv' out'
		fxch  st1						; out' cv'
		fld		st0						; out out cv
		fmul  dword [ebp + syWV2.lvol] ; outl out cv
		fxch  st1						; out outl cv
		fmul  dword [ebp + syWV2.rvol] ; outr outl cv
		fxch  st1						; outl outr cv
%ifdef FIXDENORMALS
		fadd  dword [dcoffset]
		fxch  st1
		fadd  dword [dcoffset]
		fxch  st1
%endif		
		fadd  dword [edi]		; l outr cv
		fxch  st1						; outr l cv
		fadd  dword [edi+4]	; r l cv
		fxch	st1						; l r cv
		fstp dword [edi]		; r cv
		fstp dword [edi+4]	; cv
		add esi, 4				
		add edi, 8
	dec ecx
	jnz .copyloop1
	fstp	dword [ebp + syWV2.curvol] ; -

	popad
	ret



; set
; esi: values
; ebp: workspace
syV2Set:
	pushad

  fld     dword [esi + syVV2.transp]
  fsub    dword [fc64]
  fst     dword [ebp + syWV2.xpose]

	fiadd	  dword [ebp + syWV2.note]
  fst		dword [ebp + syWV2.osc1 + syWOsc.note]
  fst		dword [ebp + syWV2.osc2 + syWOsc.note]
  fstp  dword [ebp + syWV2.osc3 + syWOsc.note]

	fld			dword [esi + syVV2.routing]
	fistp		dword [ebp + syWV2.fmode]

	fld			dword [esi + syVV2.oscsync]
	fistp		dword [ebp + syWV2.oks]

	; ... denn EQP - Panning rult.
	fld			dword [esi + syVV2.panning] ; <p>
	fmul		dword [fci128]              ; <p'>
	fld			st0                         ; <p'> <p'>
	fld1			                          ; <1> <p'> <p'>
	fsubrp	st1, st0                    ; <1-p'> <p'>
	fsqrt																; <lv> <p'>
	fstp    dword [ebp + syWV2.lvol]    ; <p'>
	fsqrt																; <rv>
	fstp    dword [ebp + syWV2.rvol] 

  ; filter balance für parallel
  fld     dword [esi + syVV2.fltbal]
  fsub    dword [fc64]
  fist    dword [temp]
  mov     eax, [temp]
  fmul    dword [fci64]             ; <x>
  or      eax, eax
  js      .fbmin
  fld1                              ; <1> <x>
  fsubr   st1, st0                  ; <1> <1-x>
  jmp     short .fbgoon
.fbmin
  fld1                              ; <1> <x>
  fadd    st1, st0                  ; <g1> <g2>
  fxch    st1                       ; <g2> <g1>
.fbgoon
  fstp    dword [ebp + syWV2.f2gain] ; <g1>
  fstp    dword [ebp + syWV2.f1gain] ; -


	lea			ebp, [ebp + syWV2.osc1 - 0]
	lea			esi, [esi + syVV2.osc1 - 0]
	call		syOscSet
	lea			ebp, [ebp + syWV2.osc2 - syWV2.osc1]
	lea			esi, [esi + syVV2.osc2 - syVV2.osc1]
	call		syOscSet
	lea			ebp, [ebp + syWV2.osc3 - syWV2.osc2]
	lea			esi, [esi + syVV2.osc3 - syVV2.osc2]
	call		syOscSet
	lea			ebp, [ebp + syWV2.aenv - syWV2.osc3]
	lea			esi, [esi + syVV2.aenv - syVV2.osc3]
	call		syEnvSet
	lea			ebp, [ebp + syWV2.env2 - syWV2.aenv]
	lea			esi, [esi + syVV2.env2 - syVV2.aenv]
	call		syEnvSet
	lea			ebp, [ebp + syWV2.vcf1 - syWV2.env2]
	lea			esi, [esi + syVV2.vcf1 - syVV2.env2]
	call		syFltSet
	lea			ebp, [ebp + syWV2.vcf2 - syWV2.vcf1]
	lea			esi, [esi + syVV2.vcf2 - syVV2.vcf1]
	call		syFltSet
	lea			ebp, [ebp + syWV2.lfo1 - syWV2.vcf2]
	lea			esi, [esi + syVV2.lfo1 - syVV2.vcf2]
	call		syLFOSet
	lea			ebp, [ebp + syWV2.lfo2 - syWV2.lfo1]
	lea			esi, [esi + syVV2.lfo2 - syVV2.lfo1]
	call		syLFOSet
	lea			ebp, [ebp + syWV2.dist - syWV2.lfo2]
	lea			esi, [esi + syVV2.dist - syVV2.lfo2]
	call		syDistSet
	popad
	ret


; note on
; eax: note
; ebx: vel
; ebp: workspace
syV2NoteOn:
  pushad
	mov		[ebp + syWV2.note], eax
	fild	dword [ebp + syWV2.note]
  fadd  dword [ebp + syWV2.xpose]

  fst		dword [ebp + syWV2.osc1 + syWOsc.note]
  fst		dword [ebp + syWV2.osc2 + syWOsc.note]
  fstp  dword [ebp + syWV2.osc3 + syWOsc.note]
	mov   [temp], ebx
	fild	dword [temp]
	fstp  dword [ebp + syWV2.velo]
	xor eax, eax
	inc eax
	mov [ebp + syWV2.gate], eax	
	; reset EGs
	mov [ebp + syWV2.aenv + syWEnv.state], eax	
	mov [ebp + syWV2.env2 + syWEnv.state], eax	

	mov eax, [ebp + syWV2.oks]
	or	eax, eax
	jz .nosync

	xor eax, eax
	mov [ebp + syWV2.osc1 + syWOsc.cnt], eax
	mov [ebp + syWV2.osc2 + syWOsc.cnt], eax
	mov [ebp + syWV2.osc3 + syWOsc.cnt], eax

.nosync

	;fldz
;	fst  dword [ebp + syWV2.curvol]
	;fstp dword [ebp + syWV2.volramp]

	lea  ebp, [ebp + syWV2.osc1 - 0]
	call syOscChgPitch
	lea  ebp, [ebp + syWV2.osc2 - syWV2.osc1]
	call syOscChgPitch
	lea  ebp, [ebp + syWV2.osc3 - syWV2.osc2]
	call syOscChgPitch

	lea  ebp, [ebp + syWV2.lfo1 - syWV2.osc3]
	call syLFOKeyOn
	lea  ebp, [ebp + syWV2.lfo2 - syWV2.lfo1]
	call syLFOKeyOn


	popad
	ret


; note off
; ebp: workspace
syV2NoteOff:
  pushad
	xor eax, eax
	mov [ebp + syWV2.gate], eax
	popad
	ret

; table for mod sources
sVTab   dd		syWV2.aenv + syWEnv.out
				dd    syWV2.env2 + syWEnv.out
				dd    syWV2.lfo1 + syWLFO.out
				dd    syWV2.lfo2 + syWLFO.out




storeV2Values:  
  pushad

	mov   ebx, [data + SYN.chanmap + 4*edx]    ; ebx = channel
	or    ebx, ebx
	jns   .doit
	jmp   .end      ; voice gar ned belegt?
.doit
	movzx eax, byte [data + SYN.chans + 8*ebx] ; pgmnummer
	mov   edi, [data + SYN.patchmap]
	mov   edi, [edi + 4*eax]							 ; edi -> sounddaten
	add   edi, [data + SYN.patchmap]

	mov   eax, edx
	imul  eax, syVV2.size
	lea   esi, [data + SYN.voicesv + eax]      ; esi -> values
	mov   eax, edx
	imul  eax, syWV2.size
	lea   ebp, [data + SYN.voicesw + eax]      ; ebp -> workspace

	; voicedependent daten übertragen
  xor   ecx, ecx
.goloop
		movzx eax, byte [edi + ecx]
		mov   [temp], eax
		fild  dword [temp]
		fstp  dword [esi + 4*ecx]
		inc   ecx
		cmp   cl, v2sound.endvdvals
	jne .goloop

	; MODMATRIX!
	movzx ecx, byte [edi + v2sound.modnum]
	lea		edi, [edi + v2sound.modmatrix]
	or	ecx, ecx
	jnz  .modloop
	jmp  .modend

.modloop
		movzx		eax, byte [edi + v2mod.source]  ; source
		or      eax, eax
		jnz     .mnotvel
		fld     dword [ebp + syWV2.velo]
		jmp			.mdo
.mnotvel
		cmp     al, 8
		jae     .mnotctl
		movzx   eax, byte [data + SYN.chans + 8*ebx + eax]
		mov     [temp], eax
		fild    dword [temp]
		jmp     .mdo
.mnotctl
    cmp     al, 12
    jae     .mnotvm
		and     al, 3
		mov     eax, [sVTab + 4*eax]
		fld     dword [ebp + eax]
		jmp     .mdo
.mnotvm
    cmp     al, 13
    jne     .mnotnote
.mnotnote
    fild    dword [ebp + syWV2.note]
    fsub    dword [fc48]
    fadd    st0, st0
    jmp     .mdo
.mdo
		movzx   eax, byte [edi + v2mod.val]
		mov     [temp], eax
		fild    dword [temp]
		fsub    dword [fc64]
		fmul    dword [fci128]
		fadd    st0, st0
		fmulp   st1, st0
		movzx   eax, byte [edi + v2mod.dest]
		cmp     eax, v2sound.endvdvals
		jb     .misok
		fstp   st0
		jmp    .mlend
.misok
		fadd    dword [esi + 4*eax]
		fstp    dword [temp]
		; clippen
		mov     edx, [temp]
		or      edx, edx
		jns     .mnoclip1
		xor     edx, edx
.mnoclip1
    cmp     edx, 43000000h
		jbe     .mnoclip2
		mov     edx, 43000000h
.mnoclip2
		mov			[esi + 4*eax], edx
.mlend
		lea edi, [edi+3]
	  dec ecx
	jz .modend
	jmp .modloop
.modend
  
	call syV2Set

.end
	popad
	ret



;#####################################################################################
; Bass, Bass, wir brauchen Bass
; BASS BOOST (fixed low shelving EQ)
;#####################################################################################

global _BASS_
_BASS_

struc syVBoost
  .amount   resd 1    ; boost in dB (0..18)
  .size
endstruc

struc syWBoost
  .ena      resd 1
  .a1       resd 1
  .a2       resd 1
  .b0       resd 1
  .b1       resd 1
  .b2       resd 1
  .x1       resd 2
  .x2       resd 2
  .y1       resd 2
  .y2       resd 2
  .size
endstruc

syBoostInit:
  pushad
  popad
  ret



; fixed frequency: 150Hz -> omega is 0,0213713785958489335949839685937381

syBoostSet:
  pushad

  fld       dword [esi + syVBoost.amount]
  fist      dword [temp]
  mov       eax, [temp]
  mov       [ebp + syWBoost.ena], eax
  or        eax, eax
  jnz       .isena
  fstp      st0
  popad
  ret

.isena
  ;    A  = 10^(dBgain/40) bzw ne stark gefakete version davon
  fmul      dword [fci128]
  call      pow2                           ; <A>
  
  ;  beta  = sqrt[ (A^2 + 1)/S - (A-1)^2 ]    (for shelving EQ filters only)
  fld       st0                            ; <A> <A>
  fmul      st0, st0                       ; <A²> <A>
  fld1                                     ; <1> <A²> <A>
  faddp     st1, st0                       ; <A²+1> <A>
  fld       st1                            ; <A> <A²+1> <A>
  fld1                                     ; <1> <A> <A²+1> <A>
  fsubp     st1, st0                       ; <A-1> <A²+1> <A>
  fmul      st0, st0                       ; <(A-1)²> <A²+1> <A>
  fsubp     st1, st0                       ; <beta²> <A>
  fsqrt                                    ; <beta> <A>

  ; zwischenvars: beta*sin, A+1, A-1, A+1*cos, A-1*cos 
  fmul      dword [SRfcBoostSin]             ; <bs> <A>
  fld1                                     ; <1> <bs> <A>
  fld       st2                            ; <A> <1> <bs> <A>
  fld       st0                            ; <A> <A> <1> <bs> <A>
  fsub      st0, st2                       ; <A-> <A> <1> <bs> <A>
  fxch      st1                            ; <A> <A-> <1> <bs> <A>
  faddp     st2, st0                       ; <A-> <A+> <bs> <A>
  fxch      st1                            ; <A+> <A-> <bs> <A>
  fld       st0                            ; <A+> <A+> <A-> <bs> <A>
  fmul      dword [SRfcBoostCos]             ; <cA+> <A+> <A-> <bs> <A>
  fld       st2                            ; <A-> <cA+> <A+> <A-> <bs> <A>
  fmul      dword [SRfcBoostCos]             ; <cA-> <cA+> <A+> <A-> <bs> <A>
  
  ;     a0 =        (A+1) + (A-1)*cos + beta*sin
  fld       st4                            ; <bs> <cA-> <cA+> <A+> <A-> <bs> <A>
  fadd      st0, st1                       ; <bs+cA-> <cA-> <cA+> <A+> <A-> <bs> <A>
  fadd      st0, st3                       ; <a0> <cA-> <cA+> <A+> <A-> <bs> <A>

  ; zwischenvar: 1/a0
  fld1                                     ; <1> <a0> <cA-> <cA+> <A+> <A-> <bs> <A>
  fdivrp    st1, st0                       ; <ia0> <cA-> <cA+> <A+> <A-> <bs> <A>

  ;     b1 =  2*A*[ (A-1) - (A+1)*cos
  fld       st4                            ; <A-> <ia0> <cA-> <cA+> <A+> <A-> <bs> <A>
  fsub      st0, st3                       ; <A- - cA+> <ia0> <cA-> <cA+> <A+> <A-> <bs> <A>
  fadd      st0, st0                       ; <2(A- - cA+)> <ia0> <cA-> <cA+> <A+> <A-> <bs> <A>
  fmul      st0, st7                       ; <b1> <ia0> <cA-> <cA+> <A+> <A-> <bs> <A>
  fmul      st0, st1                       ; <b1'> <ia0> <cA-> <cA+> <A+> <A-> <bs> <A>
  fstp      dword [ebp + syWBoost.b1]      ; <ia0> <cA-> <cA+> <A+> <A-> <bs> <A>


  ;     a1 =   -2*[ (A-1) + (A+1)*cos           
  fxch      st4                            ; <A-> <cA-> <cA+> <A+> <ia0> <bs> <A>
  faddp     st2, st0                       ; <cA-> <cA+ + A-> <A+> <ia0> <bs> <A>
  fxch      st1                            ; <cA+ + A-> <cA-> <A+> <ia0> <bs> <A>
  fadd      st0, st0                       ; <2*(cA+ + A-)> <cA-> <A+> <ia0> <bs> <A>
  fchs                                     ; <a1> <cA-> <A+> <ia0> <bs> <A>
  fmul      st0, st3                       ; <a1'> <cA-> <A+> <ia0> <bs> <A>
  fstp      dword [ebp + syWBoost.a1]      ; <cA-> <A+> <ia0> <bs> <A>

  
  ;     a2 =        (A+1) + (A-1)*cos - beta*sin
  fld       st0                            ; <cA-> <cA-> <A+> <ia0> <bs> <A>
  fadd      st0, st2                       ; <A+ + cA-> <cA-> <A+> <ia0> <bs> <A>
  fsub      st0, st4                       ; <a2> <cA-> <A+> <ia0> <bs> <A>
  fmul      st0, st3                       ; <a2'> <cA-> <A+> <ia0> <bs> <A>
  fstp      dword [ebp + syWBoost.a2]      ; <cA-> <A+> <ia0> <bs> <A>

  ;     b0 =    A*[ (A+1) - (A-1)*cos + beta*sin ]
  fsubp     st1, st0                       ; <A+ - cA-> <ia0> <bs> <A>
  fxch      st1                            ; <ia0> <A+ - cA-> <bs> <A>
  fmulp     st3, st0                       ; <A+ - cA-> <bs> <A*ia0>
  fld       st0                            ; <A+ - cA-> <A+ - cA-> <bs> <A*ia0>
  fadd      st0, st2                       ; <A+ - ca- + bs> <A+ - cA-> <bs> <A*ia0>
  fmul      st0, st3                       ; <b0'> <A+ - cA-> <bs> <A*ia0>
  fstp      dword [ebp + syWBoost.b0]      ; <A+ - cA-> <bs> <A*ia0>

  ;     b2 =    A*[ (A+1) - (A-1)*cos - beta*sin ]
  fsubrp    st1, st0                       ; <A+ - cA- - bs> <A*ia0>
  fmulp     st1, st0                       ; <b2'>
  fstp      dword [ebp + syWBoost.b2]      ; -

  popad
  ret

; esi: src/dest buffer
; ecx: # of samples

; y[n] = (b0/a0)*x[n] + (b1/a0)*x[n-1] + (b2/a0)*x[n-2] - (a1/a0)*y[n-1] - (a2/a0)*y[n-2]
syBoostProcChan:                        ; <y2> <x2> <y1> <x2>
  pushad
  .doloop
    ; y0 = b0'*in + b1'*x1 + b2'*x2 + a1'*y1 + a2'*y2
    fmul    dword [ebp + syWBoost.a2]   ; <y2a2> <x2> <y1> <x1>
    fxch    st1                         ; <x2> <y2a2> <y1> <x1>
    fmul    dword [ebp + syWBoost.b2]   ; <x2b2> <y2a2> <y1> <x1>
    fld     st3                         ; <x1> <x2b2> <y2a2> <y1> <x1>
    fmul    dword [ebp + syWBoost.b1]   ; <x1b1> <x2b2> <y2a2> <y1> <x1>
    fld     st3                         ; <y1> <x1b1> <x2b2> <y2a2> <y1> <x1>
    fmul    dword [ebp + syWBoost.a1]   ; <y1a1> <x1b1> <x2b2> <y2a2> <y1> <x1>
    fxch    st3                         ; <y2a2> <x1b1> <x2b2> <y1a1> <y1> <x1>
    fsubp   st2, st0                    ; <x1b1> <x2b2-y2a2> <y1a1> <y1> <x1>
    fsubrp  st2, st0                    ; <x2b2-y2a2> <x1b1-y1a1> <y1> <x1>
    fld     dword [esi]                 ; <x0> <x2b2-y2a2> <x1b1-y1a1> <y1> <x1>
%if FIXDENORMALS
    fadd    dword [dcoffset]
%endif        
    fxch    st4                         ; <x1> <x2b2-y2a2> <x1b1-y1a1> <y1> <x0>
    fld     st4                         ; <x0> <x1> <x2b2-y2a2> <x1b1-y1a1> <y1> <x0>
    fmul    dword [ebp + syWBoost.b0]   ; <b0x0> <x1> <x2b2-y2a2> <x1b1-y1a1> <y1> <x0>
    fxch    st2                         ; <x2b2-y2a2> <x1> <b0x0> <x1b1-y1a1> <y1> <x0>
    faddp   st3, st0                    ; <x1> <b0x0> <x1b1-y1a1+x2b2-y2a2> <y1> <x0>
    fxch    st2                         ; <x1b1-y1a1+x2b2-y2a2> <b0x0> <x1> <y1> <x0>
    faddp   st1, st0                    ; <y0> <x1> <y1> <x0>
    fst     dword [esi]    
    fxch    st2                         ; <y1> <x1> <y0> <x0>                                        
    lea     esi, [esi+8]                ; ... = <y2'> <x2'> <y1'> <x1'>
    dec     ecx
  jnz     .doloop
  popad
  ret

syBoostRender:
  pushad
  V2PerfEnter V2Perf_BASS

  test    byte [ebp + syWBoost.ena], 255
  jz      .nooo

  ; left channel
  fld     dword [ebp + syWBoost.x1]     ; <x1>
  fld     dword [ebp + syWBoost.y1]     ; <y1> <x1>
  fld     dword [ebp + syWBoost.x2]     ; <x2> <y1> <x1>
  fld     dword [ebp + syWBoost.y2]     ; <y2> <x2> <y1> <x1>
  call    syBoostProcChan
  fstp    dword [ebp + syWBoost.y2]     ; <x2'> <y1> <x1>
  fstp    dword [ebp + syWBoost.x2]     ; <y1> <x1>
  fstp    dword [ebp + syWBoost.y1]     ; <x1>
  fstp    dword [ebp + syWBoost.x1]     ; -

  lea     esi,  [esi+4]

  ; right channel
  fld     dword [ebp + syWBoost.x1 + 4] ; <x1>
  fld     dword [ebp + syWBoost.y1 + 4] ; <y1> <x1>
  fld     dword [ebp + syWBoost.x2 + 4] ; <x2> <y1> <x1>
  fld     dword [ebp + syWBoost.y2 + 4] ; <y2> <x2> <y1> <x1>
  call    syBoostProcChan
  fstp    dword [ebp + syWBoost.y2 + 4] ; <x2'> <y1> <x1>
  fstp    dword [ebp + syWBoost.x2 + 4] ; <y1> <x1>
  fstp    dword [ebp + syWBoost.y1 + 4] ; <x1>
  fstp    dword [ebp + syWBoost.x1 + 4] ; -

.nooo
  V2PerfLeave V2Perf_BASS
  popad
  ret




;#####################################################################################
; Böse Dinge, die man mit Jan Delay anstellen kann:
;  MODULATING DELAYTEIL
; (für chorus, flanger, aber auch als "großes" stereo delay. Mit Liebe verpackt, ganz für sie.)
;#####################################################################################

global _MODDEL_
_MODDEL_

struc syVModDel
  .amount   resd 1    ; dry/eff value (0=-eff, 64=dry, 127=eff)
	.fb       resd 1    ; feedback (0=-100%, 64=0%, 127=~+100%)
	.llength	resd 1		; length of left delay
	.rlength  resd 1    ; length of right delay	
	.mrate    resd 1    ; modulation rate
	.mdepth   resd 1    ; modulation depth
	.mphase   resd 1    ; modulation stereo phase (0=-180°, 64=0°, 127=180°)
	.size
endstruc

struc syWModDel
	.db1      resd 1    ; ptr: delay buffer 1
	.db2      resd 1    ; ptr: delay buffer 2
  .dbufmask resd 1    ; int: delay buffer mask

	.dbptr    resd 1    ; int: buffer write pos
	.db1offs  resd 1    ; int: buffer 1 offset
	.db2offs  resd 1    ; int: buffer 2 offset
	.mcnt     resd 1    ; mod counter
	.mfreq    resd 1    ; mod freq
	.mphase   resd 1    ; mod phase
	.mmaxoffs resd 1    ; mod max offs (2048samples*depth)

	.fbval    resd 1    ; float: feedback val
	.dryout   resd 1    ; float: dry out
	.effout   resd 1    ; float: eff out

	.size
endstruc


syModDelInit
  pushad
	xor eax, eax
	mov [ebp + syWModDel.dbptr],eax
	mov [ebp + syWModDel.mcnt],eax
	mov esi, [ebp + syWModDel.db1]
	mov edi, [ebp + syWModDel.db2]
	mov ecx, [ebp + syWModDel.dbufmask]
.clloop
	  stosd
		mov		[esi+4*ecx],eax
		dec		ecx
	jns	.clloop
	popad
	ret

syModDelSet
  pushad
	fld			dword [esi + syVModDel.amount]
	fsub		dword [fc64]
	fmul    dword [fci128]
	fadd    st0, st0
	fst     dword [ebp + syWModDel.effout]
	fabs
	fld1
	fsubrp  st1, st0
	fstp    dword [ebp + syWModDel.dryout]
	fld     dword [esi + syVModDel.fb]
	fsub		dword [fc64]
	fmul    dword [fci128]
	fadd    st0, st0
	fstp    dword [ebp + syWModDel.fbval]

  fild    dword [ebp + syWModDel.dbufmask]
	fsub    dword [fc1023]
	fmul    dword [fci128]
	fld     dword [esi + syVModDel.llength]
	fmul    st0, st1
	fistp   dword [ebp + syWModDel.db1offs]
	fld     dword [esi + syVModDel.rlength]
	fmulp   st1, st0
	fistp   dword [ebp + syWModDel.db2offs]

	fld     dword [esi + syVModDel.mrate]
	fmul    dword [fci128]
	call    calcfreq
	fmul    dword [fcmdlfomul]
	fmul    dword [SRfclinfreq]
	fistp   dword [ebp + syWModDel.mfreq]

	fld     dword [esi + syVModDel.mdepth]
	fmul    dword [fci128]
	fmul    dword [fc1023]
	fistp   dword [ebp + syWModDel.mmaxoffs]

	fld     dword [esi + syVModDel.mphase]
	fsub		dword [fc64]
	fmul    dword [fci128]
	fmul    dword [fc32bit]
	fistp   dword [ebp + syWModDel.mphase]
	shl     dword [ebp + syWModDel.mphase], 1

	popad
	ret

syModDelProcessSample  
; fpu: <r> <l> <eff> <dry> <fb>
; edx: buffer index

	push    edx

	; step 1: rechtes dingens holen
	mov     eax, [ebp + syWModDel.mcnt]
	add     eax, [ebp + syWModDel.mphase]
	shl     eax, 1
	sbb     ebx, ebx
	xor     eax, ebx
	mov     ebx, [ebp + syWModDel.mmaxoffs]
	mul     ebx  
	add     edx, [ebp + syWModDel.db2offs]
	mov     ebx, [esp]
	sub     ebx, edx
	dec     ebx
	and     ebx, [ebp + syWModDel.dbufmask]
	shr     eax, 9
	or      eax, 3f800000h
	mov     [temp], eax
	fld     dword [temp] ; <1..2> <r> <l> <eff> <dry> <fb>
	fsubr   dword [fc2]  ; <x> <r> <l> <eff> <dry> <fb>
	mov     eax,  [ebp + syWModDel.db2]
	fld     dword [eax + 4*ebx] ; <in1> <x> <r> <l> <eff> <dry> <fb>
	inc     ebx
	and     ebx, [ebp + syWModDel.dbufmask]
	fld     dword [eax + 4*ebx] ; <in2> <in1> <x> <r> <l> <eff> <dry> <fb>
	fsub    st0, st1            ; <in2-in1> <in1> <x> <r> <l> <eff> <dry> <fb>
	mov     ebx, [esp]
	fmulp   st2, st0            ; <in1> <x*(in2-in1)> <r> <l> <eff> <dry> <fb>
	faddp   st1, st0            ; <in> <r> <l> <eff> <dry> <fb>
	fld     st1                 ; <r> <in> <r> <l> <eff> <dry> <fb>
	fmul    st0, st5            ; <r*dry> <in> <r> <l> <eff> <dry> <fb>
	fld     st1                 ; <in> <r*dry> <in> <r> <l> <eff> <dry> <fb>
	fmul    st0, st5            ; <in*eff> <r*dry> <in> <r> <l> <eff> <dry> <fb>
	fxch    st2                 ; <in> <in*eff> <r*dry> <r> <l> <eff> <dry> <fb>
	fmul    st0, st7            ; <in*fb> <in*eff> <r*dry> <r> <l> <eff> <dry> <fb>
	fxch    st1                 ; <in*eff> <in*fb> <r*dry> <r> <l> <eff> <dry> <fb>
	faddp   st2, st0            ; <in*fb> <r'> <r> <l> <eff> <dry> <fb>
	faddp   st2, st0            ; <r'> <rb> <l> <eff> <dry> <fb>
	fxch    st1                 ; <rb> <r'> <l> <eff> <dry> <fb>
	fstp    dword [eax+4*ebx]   ; <r'> <l> <eff> <dry> <fb>
	fxch    st1                 ; <l> <r'> <eff> <dry> <fb>

	; step 2: linkes dingens holen
	mov     eax, [ebp + syWModDel.mcnt]
	shl     eax, 1
	sbb     ebx, ebx
	xor     eax, ebx
	mov     ebx, [ebp + syWModDel.mmaxoffs]
	mul     ebx  
	add     edx, [ebp + syWModDel.db1offs]
	mov     ebx, [esp]
	sub     ebx, edx
	and     ebx, [ebp + syWModDel.dbufmask]
	shr     eax, 9
	or      eax, 3f800000h
	mov     [temp], eax
	fld     dword [temp] ; <1..2> <l> <r'> <eff> <dry> <fb>
	fsubr   dword [fc2]  ; <x> <l> <r'> <eff> <dry> <fb>
	mov     eax,  [ebp + syWModDel.db1]
	fld     dword [eax + 4*ebx] ; <in1> <x> <l> <r'> <eff> <dry> <fb>
	inc     ebx
	and     ebx, [ebp + syWModDel.dbufmask]
	fld     dword [eax + 4*ebx] ; <in2> <in1> <x> <l> <r'> <eff> <dry> <fb>
	fsub    st0, st1            ; <in2-in1> <in1> <x> <l> <r'> <eff> <dry> <fb>
	mov     ebx, [esp]
	fmulp   st2, st0            ; <in1> <x*(in2-in1)> <l> <r'> <eff> <dry> <fb>
	faddp   st1, st0            ; <in> <l> <r'> <eff> <dry> <fb>
	fld     st1                 ; <l> <in> <l> <r'> <eff> <dry> <fb>
	fmul    st0, st5            ; <l*dry> <in> <l> <r'> <eff> <dry> <fb>
	fld     st1                 ; <in> <l*dry> <in> <l> <r'> <eff> <dry> <fb>
	fmul    st0, st5            ; <in*eff> <l*dry> <in> <l> <r'> <eff> <dry> <fb>
	fxch    st2                 ; <in> <in*eff> <l*dry> <l> <r'> <eff> <dry> <fb>
	fmul    st0, st7            ; <in*fb> <in*eff> <l*dry> <l> <r'> <eff> <dry> <fb>
	fxch    st1                 ; <in*eff> <in*fb> <l*dry> <l> <r'> <eff> <dry> <fb>
	faddp   st2, st0            ; <in*fb> <l'> <l> <r'> <eff> <dry> <fb>
	faddp   st2, st0            ; <l'> <lb> <r'> <eff> <dry> <fb>
	fxch    st1                 ; <lb> <l'> <r'> <eff> <dry> <fb>
	fstp    dword [eax+4*ebx]   ; <l'> <r'> <eff> <dry> <fb>

	pop     edx	
	mov     eax, [ebp + syWModDel.mfreq]
	add     [ebp + syWModDel.mcnt], eax
	inc     edx
	and     edx,  [ebp + syWModDel.dbufmask]
  ret


syModDelRenderAux2Main
  pushad
  V2PerfEnter V2Perf_MODDEL

	mov     eax, [ebp + syWModDel.effout]
	or      eax, eax
	jz      .dont

	fld			dword [ebp + syWModDel.fbval]					;  <fb>
	fldz                                          ;  <"dry"=0> <fb>
	fld     dword [ebp + syWModDel.effout]				;  <eff> <dry> <fb>
	mov     edx,	[ebp + syWModDel.dbptr]
	lea     esi,  [aux2buf]
.rloop
		fld     dword [esi]						;  <m> <eff> <dry> <fb>
%if FIXDENORMALS
    fadd    dword [dcoffset]
%endif    		
		fld     st0                   ;  <m> <m> <eff> <dry> <fb>
		lea     esi, [esi+4]
		call    syModDelProcessSample ;  <l'> <r'> <eff> <dry> <fb>
		fadd    dword [edi]           ;  <lout> <r'> <eff> <dry> <fb>
		fstp    dword [edi]           ;  <r'> <eff> <dry> <fb>
		fadd    dword [edi+4]         ;  <rout> <eff> <dry> <fb>
		fstp    dword [edi+4]         ;  <eff> <dry> <fb>
		lea     edi, [edi+8]

		dec     ecx
  jnz .rloop
	mov			[ebp + syWModDel.dbptr], edx
	fstp    st0			                ; <dry> <fb>
	fstp    st0                     ; <fb>
	fstp    st0                     ; -

.dont
  V2PerfLeave V2Perf_MODDEL
	popad
	ret

syModDelRenderChan
  pushad
  V2PerfEnter V2Perf_MODDEL

	mov     eax, [ebp + syWModDel.effout]
	or      eax, eax
	jz      .dont

	fld			dword [ebp + syWModDel.fbval]					;  <fb>
	fld     dword [ebp + syWModDel.dryout]				;  <dry> <fb>
	fld     dword [ebp + syWModDel.effout]				;  <eff> <dry> <fb>
	mov     edx,	[ebp + syWModDel.dbptr]
	lea     esi,  [chanbuf]
.rloop
		fld     dword [esi]						;  <l> <eff> <dry> <fb>
%if FIXDENORMALS
    fadd    dword [dcoffset]
%endif    		
		fld     dword [esi+4]					;  <r> <l> <eff> <dry> <fb>
%if FIXDENORMALS
    fadd    dword [dcoffset]
%endif    		
		call    syModDelProcessSample ;  <l'> <r'> <eff> <dry> <fb>
		fstp    dword [esi]           ;  <r'> <eff> <dry> <fb>
		fstp    dword [esi+4]         ;  <eff> <dry> <fb>
		lea     esi, [esi+8]
		dec     ecx
  jnz .rloop
	mov			[ebp + syWModDel.dbptr], edx
	fstp    st0			                ; <dry> <fb>
	fstp    st0                     ; <fb>
	fstp    st0                     ; -

.dont
  V2PerfLeave V2Perf_MODDEL
	popad
	ret




;#####################################################################################
; Für Ronny und die Freunde des Lauten:
;  STEREO COMPRESSOR
;#####################################################################################

global _COMPRESSOR_
_COMPRESSOR_

%define  COMPDLEN 5700

;PLAN:

; 1. Pegeldetection
; - entweder Peak mit fixem Falloff...
; - oder RMS über einen 8192-Samples-Buffer
; MODE: stereo/mono
; Zukunft: sidechain? low/highcut dafür?

; 2. Lookahead:
; - Delayline fürs Signal, Länge einstellbar (127msecs)

; 3. Pegelangleicher
; Werte: Threshold, Ratio (1:1 ... 1:inf), Attack (0..?), Decay (0..?)
; Zukunft:  Transientdingens (releasetime-anpassung)? Enhancer (high shelving EQ mit boost 1/reduction)?
;           Knee? (ATAN!) 

struc syVComp
  .mode         resd 1  ; 0: off / 1: Peak / 2: RMS
  .stereo       resd 1  ; 0: mono / 1: stereo
  .autogain     resd 1  ; 0: off / 1: on
  .lookahead    resd 1  ; lookahead in ms
  .threshold    resd 1  ; threshold (-54dB .. 6dB ?)
  .ratio        resd 1  ; ratio (0 : 1:1 ... 127: 1:inf)
  .attack       resd 1  ; attack value
  .release      resd 1  ; release value
  .outgain      resd 1  ; output gain 
	.size
endstruc

struc syWComp
  .mode         resd 1  ; int: mode (bit0: peak/rms, bit1: stereo, bit2: off)
  .oldmode      resd 1  ; int: last mode

  .invol        resd 1  ; flt: input gain (1/threshold, internal threshold is always 0dB)
  .ratio        resd 1  ; flt: ratio
  .outvol       resd 1  ; flt: output gain (outgain * threshold)
  .attack       resd 1  ; flt: attack   (lpf coeff, 0..1)
  .release      resd 1  ; flt: release  (lpf coeff, 0..1)
    
  .dblen        resd 1  ; int: lookahead buffer length
  .dbcnt        resd 1  ; int: lookahead buffer offset
  .curgain1     resd 1  ; flt: left current gain
  .curgain2     resd 1  ; flt: right current gain

  .pkval1       resd 1  ; flt: left/mono peak value
  .pkval2       resd 1  ; flt: right peak value
  .rmscnt       resd 1  ; int: RMS buffer offset
  .rmsval1      resd 1  ; flt: left/mono RMS current value
  .rmsval2      resd 1  ; flt: right RMS current value

  .dbuf         resd 2*COMPDLEN ; lookahead delay buffer
  .rmsbuf       resd 2*8192     ; RMS ring buffer

	.size
endstruc

syCompInit:
  pushad
  mov     al, 2
  mov     byte [ebp + syWComp.mode], al
  popad
  ret

syCompSet:
  pushad
  fld     dword [esi + syVComp.mode]
  fistp   dword [temp]
  mov     eax, [temp]
  dec     eax
  and     eax, 5
  fld     dword [esi + syVComp.stereo]
  fistp   dword [temp]
  mov     ebx,  [temp]
  add     ebx,  ebx
  add     eax,  ebx
  mov     [ebp + syWComp.mode], eax
  cmp     eax, [ebp + syWComp.oldmode]
  je      .norst
  mov     [ebp + syWComp.oldmode], eax
  mov     ecx, 2*8192
  xor     eax, eax
  mov     [ebp + syWComp.pkval1], eax
  mov     [ebp + syWComp.pkval2], eax
  mov     [ebp + syWComp.rmsval1], eax
  mov     [ebp + syWComp.rmsval2], eax
  lea     edi, [ebp + syWComp.rmsbuf]
  rep     stosd
  fld1
  fst     dword [ebp + syWComp.curgain1]
  fstp    dword [ebp + syWComp.curgain2]

.norst
  fld     dword [esi + syVComp.lookahead]
  fmul    dword [fcsamplesperms]
  fistp   dword [ebp + syWComp.dblen]
  fld     dword [esi + syVComp.threshold]
  fmul    dword [fci128]
  call    calcfreq
  fadd    st0, st0
  fadd    st0, st0
  fadd    st0, st0
  fld1
  fdiv    st0, st1
  fstp    dword [ebp + syWComp.invol]

  fld     dword [esi + syVComp.autogain]
  fistp   dword [temp]
  mov     eax,  [temp]
  or      eax,  eax
  jz      .noag
  fstp    st0
  fld1
.noag
	fld     dword [esi + syVComp.outgain]
	fsub    dword [fc64]
	fmul    dword [fci16]
	call    pow2
  fmulp   st1, st0
  fstp    dword [ebp + syWComp.outvol]

  fld     dword [esi + syVComp.ratio]
  fmul    dword [fci128]
  fstp    dword [ebp + syWComp.ratio]
   
  ;attack: 0 (!) ... 200msecs (5Hz)
  fld     dword [esi + syVComp.attack]
  fmul    dword [fci128]   ; 0 .. fast1
  fmul    dword [fcm12]    ; 0 .. fastminus12
  call    pow2             ; 1 .. 2^(fastminus12) 
  fstp    dword [ebp + syWComp.attack]

  ;release: 5ms bis 5s
  fld     dword [esi + syVComp.release]
  fmul    dword [fci128]   ; 0 .. fast1
  fmul    dword [fcm16]    ; 0 .. fastminus16
  call    pow2             ; 1 .. 2^(fastminus16) 
  fstp    dword [ebp + syWComp.release]

  popad
  ret


syCompLDMonoPeak:
  pushad
  fld     dword [ebp + syWComp.pkval1]     ; <pv>
.lp
    fld     dword [esi]           ; <l> <pv>
    fadd    dword [esi + 4]       ; <l+r> <pv>
    fmul    dword [fci2]          ; <in> <pv>
    fstp    dword [temp]          ; <pv>
    fmul    dword [fccpdfalloff]  ; <pv'>
%if FIXDENORMALS
    fadd    dword [dcoffset]
%endif        
    lea     esi,  [esi+8]
    fstp    dword [temp + 4]      ; -
    mov     eax,  [temp]
    and     eax,  7fffffffh  ; fabs()
    cmp     eax,  [temp + 4]
    jbe     .nonp
    mov     [temp + 4], eax
.nonp
    fld     dword [temp + 4]      ; <npv>
    fld     st0
    fmul    dword [ebp + syWComp.invol]
    fst     dword [edi]
    fstp    dword [edi+4]
    lea     edi,  [edi+8]
    dec     ecx
  jnz     .lp
  fstp    dword [ebp + syWComp.pkval1]     ; -
  popad
  ret

syCompLDMonoRMS:
  pushad
  fld    dword [ebp + syWComp.rmsval1]  ; <rv>
  mov    eax,  [ebp + syWComp.rmscnt]
  lea    edx,  [ebp + syWComp.rmsbuf]
.lp
    fsub    dword [edx + 4*eax]   ; <rv'>
    fld     dword [esi]           ; <l> <rv'>
    fadd    dword [esi + 4]       ; <l+r> <rv'>
    fmul    dword [fci2]          ; <in> <rv'>
%if FIXDENORMALS
    fadd    dword [dcoffset]
%endif        
    lea     esi,  [esi+8]
    fmul    st0, st0              ; <in²> <rv'>
    fadd    st1, st0              ; <in²> <rv''>
    fstp    dword [edx + 4*eax]   ; <rv''>
    fld     st0                   ; <rv''> <rv''>
    fsqrt                         ; <sqrv> <rv''>
    fmul    dword [fci8192]
    inc     eax
    and     ah, 0x1f
    fmul    dword [ebp + syWComp.invol]
    fst     dword [edi]
    fstp    dword [edi+4]         ; <rv''>
    lea     edi,  [edi+8]
    dec     ecx
  jnz     .lp
  mov     [ebp + syWComp.rmscnt], eax
  fstp    dword [ebp + syWComp.rmsval1]  ; -
  popad
  ret

syCompLDStereoPeak:
  pushad
  fld     dword [ebp + syWComp.pkval2]     ; <rpv>
  fld     dword [ebp + syWComp.pkval1]     ; <lpv> <rpv>
.lp
    fmul    dword [fccpdfalloff]  ; <lpv'> <rpv>
    fxch    st1                   ; <rpv> <lpv'>
    fmul    dword [fccpdfalloff]  ; <rpv'> <lpv'>
    fxch    st1                   ; <lpv'> <rpv'>
%if FIXDENORMALS
    fadd    dword [dcoffset]
    fxch    st1    
    fadd    dword [dcoffset]
    fxch    st1
%endif    
    fstp    dword [temp]          ; <rpv'>
    fstp    dword [temp+4]        ; -
    mov     eax,  [esi]
    and     eax,  7fffffffh       ; fabs()
    cmp     eax,  [temp]
    jbe     .nonp1
    mov     [temp], eax
.nonp1
    mov     eax,  [esi+4]
    and     eax,  7fffffffh       ; fabs()
    cmp     eax,  [temp+4]
    jbe     .nonp2
    mov     [temp+4], eax
.nonp2
    lea     esi,  [esi+8]
    fld     dword [temp+4]        ; <nrpv>
    fld     st0
    fmul    dword [ebp + syWComp.invol]
    fstp    dword [edi+4]
    fld     dword [temp]          ; <nlpv> <nrpv>
    fld     st0
    fmul    dword [ebp + syWComp.invol]
    fstp    dword [edi]
    lea     edi,  [edi+8]
    dec     ecx
  jnz     .lp
  fstp    dword [ebp + syWComp.pkval1]     ; <nrpv>
  fstp    dword [ebp + syWComp.pkval2]     ; -
  popad
  ret

syCompLDStereoRMS:
  pushad
  fld    dword [ebp + syWComp.rmsval2]  ; <rrv>
  fld    dword [ebp + syWComp.rmsval1]  ; <lrv> <rrv>
  mov    eax,  [ebp + syWComp.rmscnt]
  lea    edx,  [ebp + syWComp.rmsbuf]
.lp
    fsub    dword [edx + 8*eax]     ; <lrv'> <rrv>
    fxch    st1                     ; <rrv> <lrv'>
    fsub    dword [edx + 8*eax + 4] ; <rrv'> <lrv'>
    fxch    st1                     ; <lrv'> <rrv'>
%if FIXDENORMALS
    fadd    dword [dcoffset]
    fxch    st1    
    fadd    dword [dcoffset]
    fxch    st1
%endif    

    fld     dword [esi]             ; <l> <lrv'> <rrv'>
    fmul    st0, st0                ; <l²> <lrv'> <rrv'>
    fadd    st1, st0                ; <l²> <lrv''> <rrv'>
    fstp    dword [edx + 8*eax]     ; <lrv''> <rrv'>
    fld     st0                     ; <lrv''> <lrv''> <rrv'>
    fsqrt                           ; <sqlrv> <lrv''> <rrv'>
    fmul    dword [fci8192]
    fmul    dword [ebp + syWComp.invol]
    fstp    dword [edi]             ; <lrv''> <rrv'>

    fld     dword [esi+4]           ; <r> <lrv''> <rrv'>
    fmul    st0, st0                ; <r²> <lrv''> <rrv'>
    fadd    st2, st0                ; <r²> <lrv''> <rrv''>
    fstp    dword [edx + 8*eax + 4] ; <lrv''> <rrv''>
    fld     st1                     ; <rrv''> <lrv''> <rrv''>
    fsqrt                           ; <sqrrv> <lrv''> <rrv''>
    fmul    dword [fci8192]
    fmul    dword [ebp + syWComp.invol]
    fstp    dword [edi+4]           ; <lrv''> <rrv''>

    lea     esi,  [esi+8]
    inc     eax
    and     ah, 0x1f
    lea     edi,  [edi+8]
    dec     ecx
  jnz     .lp
  mov     [ebp + syWComp.rmscnt], eax
  fstp    dword [ebp + syWComp.rmsval1]  ; <nrrv>
  fstp    dword [ebp + syWComp.rmsval2]  ; -
  popad
  ret


; ebp: this
; ebx: ptr to lookahead line
; ecx: # of samples to process
; edx: offset into lookahead line
; esi: ptr to in/out buffer
; edi: ptr to level buffer
; st0: current gain value
syCompProcChannel:
  pushad
.cloop

    fst     dword [temp]

    ; lookahead
    fld     dword [ebx + 8*edx]         ; <v> <gain>
    fld     dword [esi]                 ; <nv> <v> <gain>
    fmul    dword [ebp + syWComp.invol] ; <nv'> <v> <gain> 
    fstp    dword [ebx + 8*edx]         ; <v> <gain>
    fmul    dword [ebp + syWComp.outvol]; <v'> <gain>
    inc     edx
    cmp     edx,  [ebp + syWComp.dblen]
    jbe     .norst
    xor     edx, edx
.norst
    
    ; destgain ermitteln
    mov     eax,  [edi]
    cmp     eax,  3f800000h               ; 1.0
    jae     .docomp
    fld1                                  ; <dgain> <v> <gain>
    jmp     .cok
.docomp
    fld     dword [edi]                   ; <lvl> <v> <gain>
    fld1                                  ; <1> <lvl> <v> <gain>
    fsubp   st1, st0                      ; <lvl-1> <v> <gain>
    fmul    dword [ebp + syWComp.ratio]   ; <r*(lvl-1)> <v> <gain>
    fld1                                  ; <1> <r*(lvl-1)> <v> <gain>
    faddp   st1, st0                      ; <1+r*(lvl-1)> <v> <gain>
    fld1                                  ; <1> <1+r*(lvl-1)> <v> <gain>
    fdivrp  st1, st0                      ; <dgain> <v> <gain>
.cok
    lea     edi,  [edi+8]

    fst     dword [temp+4]
    mov     eax,  [temp+4]
    cmp     eax,  [temp]
    jb      .attack
    fld     dword [ebp + syWComp.release] ; <spd> <dgain> <v> <gain>
    jmp     .cok2
.attack
    fld     dword [ebp + syWComp.attack]  ; <spd> <dgain> <v> <gain>
    
.cok2
    ; und compressen
    fxch    st1                           ; <dg> <spd> <v> <gain>
    fsub    st0, st3                      ; <dg-gain> <spd> <v> <gain>
    fmulp   st1, st0                      ; <spd*(dg-d)> <v> <gain>
    faddp   st2, st0                      ; <v> <gain'>
    fmul    st0, st1                      ; <out> <gain'>
    fstp    dword [esi]                   ; <gain'>
    lea     esi,  [esi+8] 

    dec     ecx
  jnz     near .cloop
  mov     [temp], edx
  popad
  ret
; on exit: [temp] = new dline count

section .dataa

syCRMTab		dd syCompLDMonoPeak, syCompLDMonoRMS, syCompLDStereoPeak, syCompLDStereoRMS

section .text

; esi: input/output buffer
; ecx: # of samples
global syCompRender
syCompRender:
  pushad
  V2PerfEnter V2Perf_COMPRESSOR

  fclex ; clear exceptions

  mov  eax, [ebp + syWComp.mode]
  test al, 4
  jz   .doit

  V2PerfLeave V2Perf_COMPRESSOR
  popad
  ret

.doit 
  ; STEP 1: level detect (fills LD buffers)

  lea     edi,  [vcebuf]
  and     al, 3
  call    [syCRMTab + 4*eax]

  ; check for FPU exception
  fstsw ax
  or    al, al
  jns   .fpuok
  ; if occured, clear LD buffer
  push ecx
  push edi
  fldz
  add  ecx, ecx
  xor  eax, eax
  rep stosd
  fstp st0
  pop  edi
  pop  ecx
.fpuok

  ; STEP 2: compress!
  lea     ebx, [ebp + syWComp.dbuf]
  mov     edx, [ebp + syWComp.dbcnt]
  fld     dword [ebp + syWComp.curgain1] 
  call    syCompProcChannel
  fstp    dword [ebp + syWComp.curgain1] 
  lea     esi, [esi+4]
  lea     edi, [edi+4]
  lea     ebx, [ebx+4]
  fld     dword [ebp + syWComp.curgain2] 
  call    syCompProcChannel
  fstp    dword [ebp + syWComp.curgain2] 
  mov     edx, [temp]
  mov     [ebp + syWComp.dbcnt], edx

  V2PerfLeave V2Perf_COMPRESSOR
  popad
  ret





;#####################################################################################
;#
;#                            E L I T E G R O U P
;#                             we are very good.
;#
;# World Domination Intro Sound System
;# -> Stereo reverb plugin (reads aux1)
;#
;# Written and (C) 1999 by The Artist Formerly Known As Doctor Roole
;#
;# This is a modified  Schroeder reverb (as found in  csound et al) consisting
;# of four parallel comb filter delay lines (with low pass filtered feedback),
;# followed by two  allpass filter delay lines  per channel. The  input signal
;# is feeded directly into half of the comb delays, while it's inverted before
;# being feeded into the other half to  minimize the response to DC offsets in
;# the incoming signal, which was a  great problem of the original implementa-
;# tion. Also, all of the comb delays are routed through 12dB/oct IIR low pass
;# filters before feeding the output  signal back to the input to simulate the
;# walls' high damping, which makes this  reverb sound a lot smoother and much
;# more realistic.
;#
;# This leaves nothing but the conclusion that we're simply better than you.
;#
;#####################################################################################

; lengths of delay lines in samples
lencl0 	equ 1309    		; left comb filter delay 0
lencl1	equ 1635		; left comb filter delay 1
lencl2 	equ 1811                ; left comb filter delay 2
lencl3 	equ 1926                ; left comb filter delay 3
lenal0 	equ 220                 ; left all pass delay 0
lenal1 	equ 74                  ; left all pass delay 1
lencr0 	equ 1327		; right comb filter delay 0
lencr1 	equ 1631                ; right comb filter delay 1
lencr2 	equ 1833                ; right comb filter delay 2
lencr3 	equ 1901                ; right comb filter delay 3
lenar0 	equ 205		; right all pass delay 0
lenar1 	equ 77		; right all pass delay 1

global _REVERB_
_REVERB_

struc syVReverb
	.revtime	resd 1
	.highcut  resd 1
  .lowcut   resd 1
	.vol      resd 1
	.size
endstruc

struc syCReverb
  .gainc0	resd 1 					; feedback gain for comb filter delay 0
  .gainc1	resd 1          ; feedback gain for comb filter delay 1
  .gainc2	resd 1          ; feedback gain for comb filter delay 2
  .gainc3	resd 1          ; feedback gain for comb filter delay 3
  .gaina0	resd 1          ; feedback gain for allpass delay 0
  .gaina1	resd 1          ; feedback gain for allpass delay 1
  .gainin	resd 1          ; input gain
  .damp		resd 1          ; high cut   (1-val²)
  .lowcut resd 1          ; low cut    (val²)
	.size
endstruc

struc syWReverb
 .setup   resb syCReverb.size
 ; positions of delay lines
 .dyn
 .poscl0 	resd 1
 .poscl1 	resd 1
 .poscl2 	resd 1
 .poscl3 	resd 1
 .posal0 	resd 1
 .posal1 	resd 1
 .poscr0 	resd 1
 .poscr1 	resd 1
 .poscr2 	resd 1
 .poscr3 	resd 1
 .posar0 	resd 1
 .posar1 	resd 1
 ; comb delay low pass filter buffers (y(k-1))
 .lpfcl0        resd 1
 .lpfcl1        resd 1
 .lpfcl2        resd 1
 .lpfcl3        resd 1
 .lpfcr0        resd 1
 .lpfcr1        resd 1
 .lpfcr2        resd 1
 .lpfcr3        resd 1
 ; memory for low cut filters
 .hpfcl     resd 1
 .hpfcr     resd 1
 ; memory for the delay lines
 .linecl0 	resd lencl0
 .linecl1 	resd lencl1
 .linecl2 	resd lencl2
 .linecl3 	resd lencl3
 .lineal0 	resd lenal0
 .lineal1 	resd lenal1
 .linecr0 	resd lencr0
 .linecr1 	resd lencr1
 .linecr2 	resd lencr2
 .linecr3 	resd lencr3
 .linear0 	resd lenar0
 .linear1 	resd lenar1

 .size
endstruc

; see struct above
syRvDefs  dd 0.966384599, 0.958186359, 0.953783929, 0.950933178, 0.994260075, 0.998044717
  				dd 1.0  ; input gain
					dd 0.8	; high cut


syReverbInit	
	pushad
	xor    eax, eax
 	mov    ecx, syWReverb.size
	mov    edi, ebp
	rep    stosb
	popad
	ret

syReverbReset
	pushad
	xor    eax, eax
 	mov    ecx, syWReverb.size-syWReverb.dyn
	lea    edi, [ebp + syWReverb.dyn]
	rep    stosb
	popad
	ret


syReverbSet
  pushad

  fld    dword [esi + syVReverb.revtime]
	fld1
	faddp  st1, st0
	fld    dword [fc64]
	fdivrp st1, st0
	fmul   st0, st0
	fmul   dword [SRfclinfreq]
	xor    ecx, ecx
.rtloop
	  fld  st0
    fld  dword [syRvDefs+4*ecx]
		call pow
		fstp dword [ebp + syWReverb.setup + syCReverb.gainc0 + 4*ecx]
		inc  ecx
		cmp  cl, 6
	jne .rtloop
	fstp   st0

	fld    dword [esi + syVReverb.highcut]
	fmul   dword [fci128]
	fmul   dword [SRfclinfreq]
	fstp   dword [ebp + syWReverb.setup + syCReverb.damp]
	fld    dword [esi + syVReverb.vol]
	fmul   dword [fci128]
	fstp   dword [ebp + syWReverb.setup + syCReverb.gainin]

	fld    dword [esi + syVReverb.lowcut]
  fmul   dword [fci128]
  fmul   st0, st0
  fmul   st0, st0
	fmul   dword [SRfclinfreq]
	fstp   dword [ebp + syWReverb.setup + syCReverb.lowcut]

	popad
	ret


syReverbProcess
  pushad
  V2PerfEnter V2Perf_REVERB

  fclex

	lea    esi,  [aux1buf]
	fld    dword [ebp + syWReverb.setup + syCReverb.lowcut]             	; <lc> <0>
	fld    dword [ebp + syWReverb.setup + syCReverb.damp]             	; <damp> <lc> <0>
  xor    ebx, ebx
	mov    eax, ecx

.sloop          ; prinzipiell nur ne große schleife
	; step 1: get input sample
	fld		 dword [esi]																  						; <in'> <damp> <lc> <0>
	fmul	 dword [ebp + syWReverb.setup + syCReverb.gainin]     	; <in> <damp> <lc> <0>
%if FIXDENORMALS	
	fadd   dword [dcoffset]
%endif	
	lea    esi,	 [esi+4]

	; step 2a: process the 4 left lpf filtered comb delays
	; left comb 0
	mov    edx,  [ebp + syWReverb.poscl0]
	fld    dword [ebp + syWReverb.linecl0+4*edx]    	; <dv> <in> <damp> <lc> <chk>
	fmul   dword [ebp + syWReverb.setup + syCReverb.gainc0]           	; <dv'> <in> <damp> <lc> <chk>
	fadd   st0,  st1                		; <nv>  <in> <damp> <lc> <chk>
	fsub   dword [ebp + syWReverb.lpfcl0]           	; <v-lp> <in> <damp> <lc> <chk>
	fmul   st0,  st2                		; <d*(v-lp)> <in> <damp> <lc> <chk>
	fadd   dword [ebp + syWReverb.lpfcl0]           	; <dout> <in> <damp> <lc> <chk>
	fst    dword [ebp + syWReverb.lpfcl0]           	; <dout> <in> <damp> <lc> <chk>
	fst    dword [ebp + syWReverb.linecl0+4*edx]    	; <asuml> <in> <damp> <lc> <chk>
	inc    edx
	cmp    dx,  lencl0
  cmove  edx, ebx
	mov    [ebp + syWReverb.poscl0], edx

	; left comb 1
	mov    edx,  [ebp + syWReverb.poscl1]
	fld    dword [ebp + syWReverb.linecl1+4*edx]    	; <dv> <asuml> <in> <damp> <lc> <chk>
	fmul   dword [ebp + syWReverb.setup + syCReverb.gainc1]           	; <dv'> <asuml> <in> <damp> <lc> <chk>
	fsub   st0,  st2                		; <nv>  <asuml> <in> <damp> <lc> <chk>
	fsub   dword [ebp + syWReverb.lpfcl1]           	; <v-lp> <asuml> <in> <damp> <lc> <chk>
	fmul   st0,  st3                		; <d*(v-lp)> <asuml> <in> <damp> <lc> <chk>
	fadd   dword [ebp + syWReverb.lpfcl1]           	; <dout> <asuml> <in> <damp> <lc> <chk>
	fst    dword [ebp + syWReverb.lpfcl1]           	; <dout> <asuml> <in> <damp> <lc> <chk>
	fst    dword [ebp + syWReverb.linecl1+4*edx]    	; <dout> <asuml> <in> <damp> <lc> <chk>
	faddp  st1,  st0                		; <asuml'> <in> <damp> <lc> <chk>
	inc    edx
	cmp    dx,  lencl1
  cmove  edx, ebx
	mov    [ebp + syWReverb.poscl1], edx

	; left comb 2
	mov    edx,  [ebp + syWReverb.poscl2]
	fld    dword [ebp + syWReverb.linecl2+4*edx]    	; <dv> <asuml> <in> <damp> <lc> <chk>
	fmul   dword [ebp + syWReverb.setup + syCReverb.gainc2]           	; <dv'> <asuml> <in> <damp> <lc> <chk>
	fadd   st0,  st2                		; <nv>  <asuml> <in> <damp> <lc> <chk>
	fsub   dword [ebp + syWReverb.lpfcl2]           	; <v-lp> <asuml> <in> <damp> <lc> <chk>
	fmul   st0,  st3                		; <d*(v-lp)> <asuml> <in> <damp> <lc> <chk>
	fadd   dword [ebp + syWReverb.lpfcl2]           	; <dout> <asuml> <in> <damp> <lc> <chk>
	fst    dword [ebp + syWReverb.lpfcl2]           	; <dout> <asuml> <in> <damp> <lc> <chk>
	fst    dword [ebp + syWReverb.linecl2+4*edx]    	; <dout> <asuml> <in> <damp> <lc> <chk>
	faddp  st1,  st0                		; <asuml'> <in> <damp> <lc> <chk>
	inc    edx
	cmp    dx,  lencl2
  cmove  edx, ebx
	mov    [ebp + syWReverb.poscl2], edx

	; left comb 3
	mov    edx,  [ebp + syWReverb.poscl3]
	fld    dword [ebp + syWReverb.linecl3+4*edx]    	; <dv> <asuml> <in> <damp> <lc> <chk>
	fmul   dword [ebp + syWReverb.setup + syCReverb.gainc3]           	; <dv'> <asuml> <in> <damp> <lc> <chk>
	fsub   st0,  st2                		; <nv>  <asuml> <in> <damp> <lc> <chk>
	fsub   dword [ebp + syWReverb.lpfcl3]           	; <v-lp> <asuml> <in> <damp> <lc> <chk>
	fmul   st0,  st3                		; <d*(v-lp)> <asuml> <in> <damp> <lc> <chk>
	fadd   dword [ebp + syWReverb.lpfcl3]           	; <dout> <asuml> <in> <damp> <lc> <chk>
	fst    dword [ebp + syWReverb.lpfcl3]           	; <dout> <asuml> <in> <damp> <lc> <chk>
	fst    dword [ebp + syWReverb.linecl3+4*edx]    	; <dout> <asuml> <in> <damp> <lc> <chk>
	faddp  st1,  st0                		; <asuml'> <in> <damp> <lc> <chk>
	inc    edx
	cmp    dx,  lencl3
  cmove  edx, ebx
	mov    [ebp + syWReverb.poscl3], edx

	; step 2b: process the 2 left allpass delays
	; left allpass 0
	mov    edx,  [ebp + syWReverb.posal0]
	fld    dword [ebp + syWReverb.lineal0+4*edx]    	; <d0v> <asuml> <in> <damp> <lc> <chk>
	fld    st0                      		; <d0v> <d0v> <asuml> <in> <damp> <lc> <chk>
	fmul   dword [ebp + syWReverb.setup + syCReverb.gaina0]           	; <d0v'> <d0v> <asuml> <in> <damp> <lc> <chk>
	faddp  st2, st0                 		; <d0v> <d0z> <in> <damp> <lc> <chk>
	fxch   st0, st1                 		; <d0z> <d0v> <in> <damp> <lc> <chk>
	fst    dword [ebp + syWReverb.lineal0+4*edx]
	fmul   dword [ebp + syWReverb.setup + syCReverb.gaina0]           	; <d0z'> <d0v> <in> <damp> <lc> <chk>
	fsubp  st1, st0                 		; <d0o> <in> <damp> <lc> <chk>
	inc    edx
  cmp    dl, lenal0
  cmove  edx, ebx
	mov    [ebp + syWReverb.posal0], edx

	; left allpass 1
	mov    edx,  [ebp + syWReverb.posal1]
	fld    dword [ebp + syWReverb.lineal1+4*edx]    	; <d1v> <d0o> <in> <damp> <lc> <chk>
	fld    st0                      		; <d1v> <d1v> <d0o> <in> <damp> <lc> <chk>
	fmul   dword [ebp + syWReverb.setup + syCReverb.gaina1]           	; <d1v'> <d1v> <d0o> <in> <damp> <lc> <chk>
	faddp  st2, st0                 		; <d1v> <d1z> <in> <damp> <lc> <chk>
	fxch   st0, st1                 		; <d1z> <d1v> <in> <damp> <lc> <chk>
	fst    dword [ebp + syWReverb.lineal1+4*edx]
	fmul   dword [ebp + syWReverb.setup + syCReverb.gaina1]           	; <d1z'> <d1v> <in> <damp> <lc> <chk>
	fsubp  st1, st0                 		; <d1o> <in> <damp> <lc> <chk>
	inc    edx
	cmp    dl, lenal1
  cmove  edx, ebx
	mov    [ebp + syWReverb.posal1], edx

  ; step 2c: low cut
	fld    dword [ebp + syWReverb.hpfcl]	; <hpf> <d1o> <in> <damp> <lc> <chk>
  fld    st0                            ; <hpf> <hpf> <d1o> <in> <damp> <lc> <chk>
  fsubr  st0, st2                       ; <d1o-hpf> <hpf> <d1o> <in> <damp> <lc> <chk>
  fmul   st0, st5                       ; <lc(d1o-hpf)> <hpf> <d1o> <in> <damp> <lc> <chk>
  faddp  st1, st0                       ; <hpf'> <d1o> <in> <damp> <lc> <chk>
  fst    dword [ebp + syWReverb.hpfcl]
  fsubp  st1, st0                       ; <outl> <in> <damp> <lc> <chk>

	; step 2d: update left mixing buffer
	fadd  dword [edi]
	fstp  dword [edi]                     ; <in> <damp> <lc> <chk>


	; step 3a: process the 4 right lpf filtered comb delays
	; right comb 0
	mov    edx,  [ebp + syWReverb.poscr0]
	fld    dword [ebp + syWReverb.linecr0+4*edx]    	; <dv> <in> <damp> <lc> <chk>
	fmul   dword [ebp + syWReverb.setup + syCReverb.gainc0]           	; <dv'> <in> <damp> <lc> <chk>
	fadd   st0,  st1                		; <nv>  <in> <damp> <lc> <chk>
	fsub   dword [ebp + syWReverb.lpfcr0]           	; <v-lp> <in> <damp> <lc> <chk>
	fmul   st0,  st2                		; <d*(v-lp)> <in> <damp> <lc> <chk>
	fadd   dword [ebp + syWReverb.lpfcr0]           	; <dout> <in> <damp> <lc> <chk>
	fst    dword [ebp + syWReverb.lpfcr0]           	; <dout> <in> <damp> <lc> <chk>
	fst    dword [ebp + syWReverb.linecr0+4*edx]    	; <asumr> <in> <damp> <lc> <chk>
	inc    edx
	cmp    dx,  lencr0
  cmove  edx, ebx
	mov    [ebp + syWReverb.poscr0], edx

	; right comb 1
	mov    edx,  [ebp + syWReverb.poscr1]
	fld    dword [ebp + syWReverb.linecr1+4*edx]    	; <dv> <asumr> <in> <damp> <lc> <chk>
	fmul   dword [ebp + syWReverb.setup + syCReverb.gainc1]           	; <dv'> <asumr> <in> <damp> <lc> <chk>
	fsub   st0,  st2                		; <nv>  <asumr> <in> <damp> <lc> <chk>
	fsub   dword [ebp + syWReverb.lpfcr1]           	; <v-lp> <asumr> <in> <damp> <lc> <chk>
	fmul   st0,  st3                		; <d*(v-lp)> <asumr> <in> <damp> <lc> <chk>
	fadd   dword [ebp + syWReverb.lpfcr1]           	; <dout> <asumr> <in> <damp> <lc> <chk>
	fst    dword [ebp + syWReverb.lpfcr1]           	; <dout> <asumr> <in> <damp> <lc> <chk>
	fst    dword [ebp + syWReverb.linecr1+4*edx]    	; <dout> <asumr> <in> <damp> <lc> <chk>
	faddp  st1,  st0                		; <asumr'> <in> <damp> <lc> <chk>
	inc    edx
	cmp    dx,  lencr1
  cmove  edx, ebx
	mov    [ebp + syWReverb.poscr1], edx

	; right comb 2
	mov    edx,  [ebp + syWReverb.poscr2]
	fld    dword [ebp + syWReverb.linecr2+4*edx]    	; <dv> <asumr> <in> <damp> <lc> <chk>
	fmul   dword [ebp + syWReverb.setup + syCReverb.gainc2]           	; <dv'> <asumr> <in> <damp> <lc> <chk>
	fadd   st0,  st2                		; <nv>  <asumr> <in> <damp> <lc> <chk>
	fsub   dword [ebp + syWReverb.lpfcr2]           	; <v-lp> <asumr> <in> <damp> <lc> <chk>
	fmul   st0,  st3                		; <d*(v-lp)> <asumr> <in> <damp> <lc> <chk>
	fadd   dword [ebp + syWReverb.lpfcr2]           	; <dout> <asumr> <in> <damp> <lc> <chk>
	fst    dword [ebp + syWReverb.lpfcr2]           	; <dout> <asumr> <in> <damp> <lc> <chk>
	fst    dword [ebp + syWReverb.linecr2+4*edx]    	; <dout> <asumr> <in> <damp> <lc> <chk>
	faddp  st1,  st0                		; <asumr'> <in> <damp> <lc> <chk>
	inc    edx
	cmp    dx,  lencr2
  cmove  edx, ebx
	mov    [ebp + syWReverb.poscr2], edx

	; right comb 3
	mov    edx,  [ebp + syWReverb.poscr3]
	fld    dword [ebp + syWReverb.linecr3+4*edx]    	; <dv> <asumr> <in> <damp> <lc> <chk>
	fmul   dword [ebp + syWReverb.setup + syCReverb.gainc3]           	; <dv'> <asumr> <in> <damp> <lc> <chk>
	fsubrp st2,  st0                		; <asumr> <nv> <damp> <lc> <chk>
	fxch   st0,  st1                		; <nv> <asumr> <damp> <lc> <chk>
	fsub   dword [ebp + syWReverb.lpfcr3]           	; <v-lp> <asumr> <damp> <lc> <chk>
	fmul   st0,  st2                		; <d*(v-lp)> <asumr> <damp> <lc> <chk>
	fadd   dword [ebp + syWReverb.lpfcr3]           	; <dout> <asumr> <damp> <lc> <chk>
	fst    dword [ebp + syWReverb.lpfcr3]           	; <dout> <asumr> <damp> <lc> <chk>
	fst    dword [ebp + syWReverb.linecr3+4*edx]    	; <dout> <asumr> <damp> <lc> <chk>
	faddp  st1,  st0                		; <asumr'> <damp> <lc> <chk>
	inc    edx
	cmp    dx,  lencr3
  cmove  edx, ebx
	mov    [ebp + syWReverb.poscr3], edx


	; step 2b: process the 2 right allpass delays
	; right allpass 0
	mov    edx,  [ebp + syWReverb.posar0]
	fld    dword [ebp + syWReverb.linear0+4*edx]    	; <d0v> <asumr> <damp> <lc> <chk>
	fld    st0                      		; <d0v> <d0v> <asumr> <damp> <lc> <chk>
	fmul   dword [ebp + syWReverb.setup + syCReverb.gaina0]           	; <d0v'> <d0v> <asumr> <damp> <lc> <chk>
	faddp  st2, st0                 		; <d0v> <d0z> <damp> <lc> <chk>
	fxch   st0, st1                 		; <d0z> <d0v> <damp> <lc> <chk>
	fst    dword [ebp + syWReverb.linear0+4*edx]
	fmul   dword [ebp + syWReverb.setup + syCReverb.gaina0]           	; <d0z'> <d0v> <damp> <lc> <chk>
	fsubp  st1, st0                 		; <d0o> <damp> <lc> <chk>
	inc    edx
	cmp    dl, lenar0
  cmove  edx, ebx
	mov    [ebp + syWReverb.posar0], edx

	; right allpass 1
	mov    edx,  [ebp + syWReverb.posar1]
	fld    dword [ebp + syWReverb.linear1+4*edx]    	; <d1v> <d0o> <damp> <lc> <chk>
	fld    st0                      		; <d1v> <d1v> <d0o> <damp> <lc> <chk>
	fmul   dword [ebp + syWReverb.setup + syCReverb.gaina1]           	; <d1v'> <d1v> <d0o> <damp> <lc> <chk>
	faddp  st2, st0                 		; <d1v> <d1z> <damp> <lc> <chk>
	fxch   st0, st1                 		; <d1z> <d1v> <damp> <lc> <chk>
	fst    dword [ebp + syWReverb.linear1+4*edx]
	fmul   dword [ebp + syWReverb.setup + syCReverb.gaina1]           	; <d1z'> <d1v> <damp> <lc> <chk>
	fsubp  st1, st0                 		; <d1o> <damp> <lc> <chk>
	inc    edx
	cmp    dl, lenar1
  cmove  edx, ebx
	mov    [ebp + syWReverb.posar1], edx


  ; step 2c: low cut
	fld    dword [ebp + syWReverb.hpfcr]	; <hpf> <d1o> <damp> <lc> <chk>
  fld    st0                            ; <hpf> <hpf> <d1o>  <damp> <lc> <chk>
  fsubr  st0, st2                       ; <d1o-hpf> <hpf> <d1o> <damp> <lc> <chk>
  fmul   st0, st4                       ; <lc(d1o-hpf)> <hpf> <d1o> <damp> <lc> <chk>
  faddp  st1, st0                       ; <hpf'> <d1o> <damp> <lc> <chk>
  fst    dword [ebp + syWReverb.hpfcr]
  fsubp  st1, st0                       ; <outr> <damp> <lc> <chk>

	; step 2d: update left mixing buffer
	fadd  dword [edi+4]
	fstp  dword [edi+4]                   ; <damp> <lc> <chk>


	lea    edi,  [edi+8]
	dec    ecx
	jnz    near .sloop

	fstp   st0                      ; <lc> <chk>
	fstp   st0                      ; <chk>

	; FPU underflow protection
	fstsw  ax
	and    eax, byte 16
	jz     .dontpanic
	call   syReverbReset


.dontpanic

  V2PerfLeave V2Perf_REVERB
	popad
	ret

;#####################################################################################
; Oberdings
; CHANNELS RULEN
;#####################################################################################

global _V2CHAN_
_V2CHAN_

struc syVChan
	.chanvol resd 1
	.auxarcv resd 1			; CHAOS aux a receive
	.auxbrcv resd 1			; CHAOS aux b receive
	.auxasnd resd 1			; CHAOS aux a send
	.auxbsnd resd 1			; CHAOS aux b send
	.aux1		 resd 1
	.aux2    resd 1
	.fxroute resd 1
  .boost   resb syVBoost.size
	.cdist   resb syVDist.size
	.chorus  resb syVModDel.size
  .compr   resb syVComp.size
	.size
endstruc

struc syWChan
	.chgain resd 1
	.a1gain resd 1
	.a2gain resd 1
	.aasnd  resd 1
	.absnd  resd 1
	.aarcv  resd 1
	.abrcv  resd 1
	.fxr    resd 1
  .boostw resb syWBoost.size
	.distw  resb syWDist.size
	.chrw		resb syWModDel.size
  .compw  resb syWComp.size
	.size
endstruc


; ebp: wchan
syChanInit
  pushad
	lea		ebp, [ebp + syWChan.distw]
	call	syDistInit
	lea		ebp, [ebp + syWChan.chrw - syWChan.distw]
	call	syModDelInit
	lea		ebp, [ebp + syWChan.compw - syWChan.chrw]
  call  syCompInit
	lea		ebp, [ebp + syWChan.boostw - syWChan.compw]
  call  syBoostInit
	popad
	ret

; esi: vchan
; ebp: wchan
syChanSet
  pushad
  fld		dword [esi + syVChan.auxarcv]
	fmul	dword [fci128]
	fstp	dword [ebp + syWChan.aarcv]
  fld		dword [esi + syVChan.auxbrcv]
	fmul	dword [fci128]
	fstp	dword [ebp + syWChan.abrcv]
  fld		dword [esi + syVChan.auxasnd]
	fmul	dword [fci128]
	fmul	dword [fcgain]
	fstp	dword [ebp + syWChan.aasnd]
  fld		dword [esi + syVChan.auxbsnd]
	fmul	dword [fci128]
	fmul	dword [fcgain]
	fstp	dword [ebp + syWChan.absnd]
  fld		dword [esi + syVChan.chanvol]
	fmul	dword [fci128]
	fmul	dword [fcgain]
	fst 	dword [ebp + syWChan.chgain]
  fld		dword [esi + syVChan.aux1]
	fmul	dword [fci128]
	fmul	dword [fcgainh]
	fmul  st0, st1
	fstp	dword [ebp + syWChan.a1gain]
  fld		dword [esi + syVChan.aux2]
	fmul	dword [fci128]
	fmul	dword [fcgainh]
	fmulp st1, st0
	fstp	dword [ebp + syWChan.a2gain]
	fld		dword [esi + syVChan.fxroute]
	fistp dword [ebp + syWChan.fxr]
	lea   esi,  [esi + syVChan.cdist]
	lea		ebp,	[ebp + syWChan.distw]
	call	syDistSet
	lea   esi,  [esi + syVChan.chorus - syVChan.cdist]
	lea		ebp,	[ebp + syWChan.chrw - syWChan.distw]
	call	syModDelSet
	lea   esi,  [esi + syVChan.compr - syVChan.chorus]
	lea		ebp,  [ebp + syWChan.compw - syWChan.chrw]
	call	syCompSet
	lea   esi,  [esi + syVChan.boost - syVChan.compr]
	lea		ebp,  [ebp + syWChan.boostw - syWChan.compw]
	call	syBoostSet
	popad 
	ret

syChanProcess
  pushad

	; AuxA Receive		CHAOS
	mov   eax, [ebp + syWChan.aarcv]
	or    eax, eax
	jz    .noauxarcv
	push	ecx
	mov		edi, chanbuf
	mov   esi, auxabuf
.auxarcvloop
		fld		dword [esi]
		fmul  dword [ebp + syWChan.aarcv]
		fld		dword [esi+4]
		fmul  dword [ebp + syWChan.aarcv]
		lea   esi, [esi + 8]
		fxch  st1
		fadd	dword [edi]
		fxch  st1
		fadd	dword [edi+4]
		fxch  st1
		fstp  dword [edi]
		fstp  dword [edi+4]
		lea   edi, [edi + 8]
		dec ecx
	jnz		.auxarcvloop
	pop		ecx
.noauxarcv

	; AuxB Receive		CHAOS
	mov   eax, [ebp + syWChan.abrcv]
	or    eax, eax
	jz    .noauxbrcv
	push	ecx
	mov		edi, chanbuf
	mov   esi, auxbbuf
.auxbrcvloop
		fld		dword [esi]
		fmul  dword [ebp + syWChan.abrcv]
		fld		dword [esi+4]
		fmul  dword [ebp + syWChan.abrcv]
		lea   esi, [esi + 8]
		fxch  st1
		fadd	dword [edi]
		fxch  st1
		fadd	dword [edi+4]
		fxch  st1
		fstp  dword [edi]
		fstp  dword [edi+4]
		lea   edi, [edi + 8]
		dec ecx
	jnz		.auxbrcvloop
	pop		ecx
.noauxbrcv



	mov		esi, chanbuf
	mov		edi, esi

	lea		ebp, [ebp + syWChan.compw - 0]
  call  syCompRender  
	lea		ebp, [ebp + syWChan.boostw - syWChan.compw]
  call  syBoostRender  
	lea		ebp, [ebp + 0 - syWChan.boostw]

	mov   eax, [ebp + syWChan.fxr]
	or    eax, eax
	jnz   .otherway
	lea		ebp, [ebp - 0 + syWChan.distw]
	call	syDistRenderStereo
	lea		ebp, [ebp - syWChan.distw + syWChan.chrw]
	call	syModDelRenderChan
	lea		ebp, [ebp - syWChan.chrw + 0]
	jmp   short .weiterso
.otherway
	lea		ebp, [ebp + syWChan.chrw - 0]
	call	syModDelRenderChan
	lea		ebp, [ebp + syWChan.distw - syWChan.chrw]
	call	syDistRenderStereo
	lea		ebp, [ebp + 0 - syWChan.distw]

.weiterso

	; Aux1...
	mov   eax, [ebp + syWChan.a1gain]
	or    eax, eax
	jz    .noaux1

	push	ecx
	push  esi
	lea   edi, [aux1buf]
.aux1loop
		fld		dword [esi]
		fadd	dword [esi+4]
		lea   esi, [esi+8]
		fmul  dword [ebp + syWChan.a1gain]
		fadd  dword [edi]
		fstp  dword [edi]
		lea   edi, [edi+4]
		dec		ecx
	jnz		.aux1loop
	pop   esi
	pop		ecx

.noaux1

	; ... und Aux2.
	mov   eax, [ebp + syWChan.a2gain]
	or    eax, eax
	jz    .noaux2
	push	ecx
	push  esi
	lea   edi, [aux2buf]
.aux2loop
		fld		dword [esi]
		fadd	dword [esi+4]
		lea   esi, [esi+8]
		fmul  dword [ebp + syWChan.a2gain]
		fadd  dword [edi]
		fstp  dword [edi]
		lea   edi, [edi+4]
		dec		ecx
	jnz		.aux2loop
	pop   esi
	pop		ecx

.noaux2

	; AuxA...		CHAOS
	mov   eax, [ebp + syWChan.aasnd]
	or    eax, eax
	jz    .noauxa
	push	ecx
	push  esi
	mov   edi, auxabuf
.auxaloop
		fld		dword [esi]
		fmul  dword [ebp + syWChan.aasnd]
		fld		dword [esi+4]
		fmul  dword [ebp + syWChan.aasnd]
		lea   esi, [esi + 8]
		fxch  st1
		fadd	dword [edi]
		fxch  st1
		fadd	dword [edi+4]
		fxch  st1
		fstp  dword [edi]
		fstp  dword [edi+4]
		lea   edi, [edi + 8]
		dec ecx
	jnz		.auxaloop
	pop   esi
	pop		ecx
.noauxa

	; AuxB...		CHAOS
	mov   eax, [ebp + syWChan.absnd]
	or    eax, eax
	jz    .noauxb
	push	ecx
	push  esi
	mov   edi, auxbbuf
.auxbloop
		fld		dword [esi]
		fmul  dword [ebp + syWChan.absnd]
		fld		dword [esi+4]
		fmul  dword [ebp + syWChan.absnd]
		lea   esi, [esi + 8]
		fxch  st1
		fadd	dword [edi]
		fxch  st1
		fadd	dword [edi+4]
		fxch  st1
		fstp  dword [edi]
		fstp  dword [edi+4]
		lea   edi, [edi + 8]
		dec ecx
	jnz		.auxbloop
	pop   esi
	pop		ecx
.noauxb

  ; Chanbuf in Mainbuffer kopieren
	mov		edi, mixbuf
.ccloop			
		fld		dword [esi]
		fmul  dword [ebp + syWChan.chgain]
		fld		dword [esi+4]
		fmul  dword [ebp + syWChan.chgain]
		lea   esi, [esi + 8]
		fxch  st1
		fadd	dword [edi]
		fxch  st1
		fadd	dword [edi+4]
		fxch  st1
		fstp  dword [edi]
		fstp  dword [edi+4]
		lea   edi, [edi + 8]
		dec ecx
	jnz .ccloop

	popad
	ret


; ebx: channel

storeChanValues:  
  pushad

	movzx eax, byte [data + SYN.chans + 8*ebx] ; pgmnummer
	mov   edi, [data + SYN.patchmap]
	mov   edi, [edi + 4*eax]									; edi -> sounddaten
	add   edi, [data + SYN.patchmap]

	mov   eax, ebx
	imul  eax, syVChan.size
	lea   esi, [data + SYN.chansv + eax]      ; esi -> values
	mov   eax, ebx
	imul  eax, syWChan.size
	lea   ebp, [data + SYN.chansw + eax]      ; ebp -> workspace
	push  ebp
	
	; channeldependent daten übertragen
	push esi
	xor  ecx, ecx
	mov  cl, v2sound.chan
.goloop
		movzx eax, byte [edi + ecx]
		mov   [temp], eax
		fild  dword [temp]
		fstp  dword [esi]
		lea   esi, [esi+4]
		inc   ecx
		cmp   cl, v2sound.endcdvals
	jne .goloop
	pop esi

  ; ebp: ptr auf voice
	mov   eax, [data + SYN.voicemap + 4*ebx]
	imul  eax, syWV2.size
	lea   ebp, [data + SYN.voicesw + eax]      ; ebp -> workspace


	; MODMATRIX!
	movzx ecx, byte [edi + v2sound.modnum]
	lea		edi, [edi + v2sound.modmatrix]
	or		ecx, ecx
	jnz  .modloop
	jmp  .modend

.modloop
		movzx   eax, byte [edi + v2mod.dest]
		cmp     al, v2sound.chan
		jb      .mlegw
		cmp     al, v2sound.endcdvals
		jb      .mok
.mlegw
		jmp     .mlend
.mok
		movzx		eax, byte [edi + v2mod.source]  ; source
		or      eax, eax
		jnz     .mnotvel
		fld     dword [ebp + syWV2.velo]
		jmp			.mdo
.mnotvel
		cmp     al, 8
		jae     .mnotctl		
;		jae     .mlend
		movzx   eax, byte [data + SYN.chans + 8*ebx + eax]
		mov     [temp], eax
		fild    dword [temp]
		jmp     .mdo
.mnotctl
    cmp     al, 12
    jae     .mnotvm
		and     al, 3
		mov     eax, [sVTab + 4*eax]
		fld     dword [ebp + eax]
		jmp     .mdo
.mnotvm
    cmp     al, 13
    jne     .mnotnote
.mnotnote
    fild    dword [ebp + syWV2.note]
    fsub    dword [fc48]
    fadd    st0, st0		
.mdo
		movzx   eax, byte [edi + v2mod.val]
		mov     [temp], eax
		fild    dword [temp]
		fsub    dword [fc64]
		fmul    dword [fci128]
		fadd    st0, st0
		fmulp   st1, st0
		movzx   eax, byte [edi + v2mod.dest]
		sub     al, v2sound.chan
		fadd    dword [esi + 4*eax]
		fstp    dword [temp]
		; clippen
		mov     edx, [temp]
		or      edx, edx
		jns     .mnoclip1
		xor     edx, edx
.mnoclip1
    cmp     edx, 43000000h
		jbe     .mnoclip2
		mov     edx, 43000000h
.mnoclip2
		mov			[esi + 4*eax], edx
.mlend
		lea edi, [edi+3]
	  dec ecx
	jz .modend
	jmp .modloop
.modend
  
  pop ebp
	call syChanSet

.end
	popad
	ret



;#####################################################################################
;
; RONAN, DER FREUNDLICHE SPRECHROBOTER
;
;#####################################################################################


; FILTERBANK:

%ifdef RONAN

global _RONAN_
_RONAN_

extern _ronanCBInit@0
extern _ronanCBTick@0
extern _ronanCBNoteOn@0
extern _ronanCBNoteOff@0
extern _ronanCBSetCtl@8
extern _ronanCBProcess@8
extern _ronanCBProcess@8
extern _ronanCBSetSR@4

; ebp: this
syRonanInit
	pushad
	call _ronanCBInit@0
	popad
	ret

; ebp: this
syRonanNoteOn
	pushad
	call _ronanCBNoteOn@0
	popad
	ret

; ebp: this
syRonanNoteOff
	pushad
	call _ronanCBNoteOff@0
	popad
	ret

; ebp: this
syRonanTick
	pushad
	call _ronanCBTick@0
	popad
	ret

; ebp: this
; esi: buffer
; ecx: count
syRonanProcess
	pushad
	V2PerfEnter V2Perf_RONAN
	push    ecx
	push    esi
	call    _ronanCBProcess@8
	V2PerfLeave V2Perf_RONAN
	popad
	ret


%endif

;#####################################################################################
; Oberdings
; SOUNDDEFINITIONEN FÜR DEN WELTFRIEDEN
;#####################################################################################

struc v2sound
   ; voice dependent sachen

	.voice			resb syVV2.size/4
	.endvdvals

  ; globale pro-channel-sachen
	.chan				resb syVChan.size/4
	.endcdvals

	; manuelles rumgefake
  .maxpoly		resb 1

	; modmatrix
	.modnum			resb 1
	.modmatrix
	.size
endstruc

struc v2mod
	.source   resb 1   ; source: vel/ctl1-7/aenv/env2/lfo1/lfo2
	.val      resb 1   ; 0 = -1 .. 128=1
	.dest     resb 1   ; destination (index into v2sound)
	.size     
endstruc



;#####################################################################################
; Oberdings
; ES GEHT LOS
;#####################################################################################

section .bss

struc CHANINFO
  .pgm      resb 1
	.ctl      resb 7
	.size
endstruc

struc SYN
	.patchmap resd 1
	.mrstat   resd 1
	.curalloc resd 1
	.samplerate resd 1
	.chanmap	resd POLY
	.allocpos resd POLY
	.chans    resb 16*CHANINFO.size
	.voicemap resd 16

	.tickd    resd 1

  .voicesv  resb POLY*syVV2.size
  .voicesw  resb POLY*syWV2.size

  .chansv		resb 16*syVChan.size
  .chansw		resb 16*syWChan.size

	.globalsstart
	.rvbparm  resb syVReverb.size
	.delparm  resb syVModDel.size
	.vlowcut  resd 1
	.vhighcut resd 1
  .cprparm  resb syVComp.size
	.guicolor	resb 1			; CHAOS gui logo color (has nothing to do with sound but should be saved with patch)
	.globalsend

	.reverb   resb syWReverb.size
	.delay    resb syWModDel.size
	.compr    resb syWComp.size
	.lcfreq   resd 1
	.lcbufl   resd 1
	.lcbufr   resd 1
	.hcfreq   resd 1
	.hcbufl   resd 1
	.hcbufr   resd 1

%ifdef VUMETER
  .vumode   resd 1
  .chanvu   resd 2*16
  .mainvu   resd 2
%endif  

	.size
endstruc

global _DATA_
_DATA_
data			resb SYN.size


section .text


;-------------------------------------------------------------------------------------
; Init

global _synthInit@8
_synthInit@8:
		pushad

%ifdef RONAN		
		mov eax, [esp + 40]
		push eax
    call _ronanCBSetSR@4
%endif    

		xor eax, eax
		mov ecx, SYN.size
		mov edi, data
		rep stosb

		mov eax, [esp + 40]
		mov [data + SYN.samplerate], eax
		call calcNewSampleRate
		
		mov eax, [esp + 36]
		mov [data + SYN.patchmap], eax

		mov cl, POLY
		mov ebp, data + SYN.voicesw
.siloop1
      call	syV2Init
			lea		ebp, [ebp+syWV2.size]
			mov   dword [data + SYN.chanmap - 4 + 4*ecx], -1
			dec   ecx
		jnz .siloop1

		mov cl, 0
		mov ebp, data + SYN.chansw
.siloop2
			mov   byte [data + SYN.chans + 8*ecx + 7], 7fh
			mov   eax, ecx
			shl   eax, 14
			lea   eax, [chandelbuf + eax]
			mov   [ebp + syWChan.chrw + syWModDel.db1], eax
			add   eax, 8192
			mov   [ebp + syWChan.chrw + syWModDel.db2], eax
			mov   eax, 2047
			mov   [ebp + syWChan.chrw + syWModDel.dbufmask], eax
      call	syChanInit
			lea		ebp, [ebp+syWChan.size]
			inc   ecx
			cmp   cl, 16
		jne .siloop2

		lea		ebp, [data + SYN.reverb]
		call  syReverbInit

		lea		ebp, [data + SYN.delay]
		lea   eax, [maindelbuf]
		mov   [ebp + syWModDel.db1], eax
		add   eax, 131072
		mov   [ebp + syWModDel.db2], eax
		mov   eax, 32767
		mov   [ebp + syWModDel.dbufmask], eax
    call	syModDelInit

%ifdef RONAN
		call  syRonanInit
%endif

    lea   ebp, [data + SYN.compr]
    call  syCompInit

		popad
		ret 8


;-------------------------------------------------------------------------------------
; Renderloop!

section .bss

todo	 resd 2
outptr resd 2
addflg resd 1

section .text


global _synthRender@16
_synthRender@16:
		pushad

		; FPUsetup...
		fstcw	[oldfpcw]
		mov		ax, [oldfpcw]
		and		ax, 0f0ffh
		or		ax, 3fh
		mov		[temp], ax
		finit
	 	fldcw	[temp]

		mov   ecx, [esp+40]
		mov   [todo], ecx
		mov   edi, [esp+48]
		mov   [addflg], edi
		mov   edi, [esp+44]
		mov   [outptr+4], edi
		mov   edi, [esp+36]
		mov   [outptr], edi

.fragloop:
			; step 1: fragmentieren
      mov   eax, [todo]
		  or    eax, eax
		  jnz   .doit

		  fldcw [oldfpcw] ; nix zu tun? -> ende
		  popad
		  ret 16
.doit

			; neuer Frame nötig?
			mov   ebx, [data + SYN.tickd]
			or    ebx, ebx
			jnz   .nonewtick

	    ; tick abspielen
			xor   edx, edx
.tickloop 
        mov		eax, [data + SYN.chanmap + 4*edx]
				or		eax, eax
				js		.tlnext
				mov		eax, edx
				imul	eax, syWV2.size
				lea   ebp, [data + SYN.voicesw + eax]
				call  storeV2Values
				call  syV2Tick
				; checken ob EG ausgelaufen
				mov   eax, [ebp + syWV2.aenv + syWEnv.state]
				or    eax, eax
				jnz   .stillok
				; wenn ja -> ausmachen!
				not   eax
        mov		[data + SYN.chanmap + 4*edx], eax
				jmp   .tlnext
.stillok
.tlnext 
				inc   edx
				cmp   dl, POLY
			jne .tickloop

			xor   ebx, ebx
.tickloop2
				call  storeChanValues
				inc   ebx
				cmp   bl, 16
			jne		.tickloop2

%ifdef RONAN
			call  syRonanTick
%endif

			; frame rendern
			mov   ecx,	dword [SRcFrameSize]
			mov   [data + SYN.tickd], ecx
			call  .RenderBlock

.nonewtick 

      ; daten in destination umkopieren
			mov   eax, [todo]
			mov   ebx, [data + SYN.tickd]
			mov   ecx, ebx

			mov   esi, [SRcFrameSize]
			sub   esi, ecx
			shl   esi, 3
			add   esi, mixbuf

			cmp   eax, ecx
		  jge   .tickdg
			mov   ecx, eax
.tickdg
			sub   eax, ecx
			sub   ebx, ecx
			mov   [todo], eax
			mov   [data + SYN.tickd], ebx
			
			mov   edi, [outptr]			
			mov   ebx, [outptr+4]
			or    ebx, ebx
			jz    .interleaved

			; separate channels
			mov   eax, [addflg]
			or    eax, eax
			jz    .seploopreplace

			.seploopadd
			  mov eax, [edi]
				mov edx, [ebx]
				add eax, [esi]
				add edx, [esi+4]
				lea esi, [esi+8]
				mov [edi], eax
				lea edi, [edi+4]
				mov [ebx], edx
				lea ebx, [ebx+4]
				dec ecx
		  jnz .seploopadd
			jz  .seploopend

			.seploopreplace
				mov eax, [esi]
				mov edx, [esi+4]
				lea esi, [esi+8]
				mov [edi], eax
				lea edi, [edi+4]
				mov [ebx], edx
				lea ebx, [ebx+4]
				dec ecx
		  jnz .seploopreplace
			
			.seploopend
			mov [outptr], edi
			mov [outptr+4], ebx
			jmp .fragloop


.interleaved
			shl   ecx, 1
			rep   movsd
			mov   [outptr], edi

      jmp		.fragloop



.RenderBlock
		pushad
		V2PerfClear
		V2PerfEnter V2Perf_TOTAL
		
		; clear output buffer
		mov   [todo+4], ecx
		mov   edi, mixbuf
		xor   eax, eax
		shl   ecx, 1
		rep   stosd

		; clear aux1
		mov   ecx, [todo+4]
		mov   edi, aux1buf
		rep   stosd

		; clear aux2
		mov   ecx, [todo+4]
		mov   edi, aux2buf
		rep   stosd

		; clear aux a / b
		mov 	ecx, [todo+4]
		add		ecx,ecx
		mov		eax,ecx
		mov		edi, auxabuf
		rep		stosd
		mov 	ecx,eax
		mov		edi, auxbbuf
		rep		stosd


    ; process all channels
		xor   ecx, ecx
.chanloop
      push ecx
			; check if voices are active
			xor edx, edx		
.cchkloop
				cmp ecx, [data + SYN.chanmap + 4*edx]
				je  .dochan
				inc edx
				cmp dl, POLY
			jne .cchkloop
%ifdef VUMETER
      xor		eax, eax
      mov		[data + SYN.chanvu + 8*ecx], eax
      mov		[data + SYN.chanvu + 8*ecx + 4], eax   
%endif						
			jmp .chanend
.dochan
			; clear channel buffer
			mov edi, chanbuf
			mov ecx, [todo+4]
			shl ecx, 1
			xor eax, eax
			rep stosd

			; process all voices belonging to that channel
			xor		edx, edx
.vceloop
				pop		ecx
				push	ecx
				cmp		ecx, [data + SYN.chanmap + 4*edx]
				jne		.vceend

				; alle Voices rendern
				mov		eax, edx
				imul	eax, syWV2.size
				lea   ebp, [data + SYN.voicesw + eax]
				mov   ecx, [todo+4]
				call	syV2Render
				
.vceend 
				inc edx
				cmp dl, POLY
			jne .vceloop

%ifdef RONAN
			; Wenn channel 16, dann Sprachsynth!
			pop		ecx
			push	ecx
			cmp   cl, 15
			jne   .noronan
	
			mov		ecx, [todo+4]
			lea   esi, [chanbuf]
			call  syRonanProcess
.noronan
%endif
			
			pop		ecx
			push	ecx
			imul	ecx, syWChan.size
			lea   ebp, [data + SYN.chansw + ecx]
			mov		ecx, [todo+4]
			call	syChanProcess

%ifdef VUMETER
		  ; VU-Meter der Channels bearbeiten
		  pop		edx
		  push	edx
		  mov   esi, chanbuf
		  lea   edi, [data + SYN.chanvu + 8*edx]
		  call  .vumeter
%endif		  
			
.chanend
			pop ecx
			inc ecx
			cmp cl, 16
		je .clend
		jmp .chanloop

.clend

		
		; Reverb/Delay draufrechnen
		mov		ecx, [todo+4]
		lea		ebp, [data + SYN.reverb]
		mov		edi, mixbuf
		call	syReverbProcess
		lea   ebp, [data + SYN.delay]
		call  syModDelRenderAux2Main



		; lowcut/highcut
		mov		ecx, [todo+4]
		mov		esi, mixbuf
		mov   eax, [data + SYN.hcfreq]
		fld   dword [data + SYN.hcbufr]   ; <hcbr>
		fld   dword [data + SYN.lcbufr]   ; <lcbr> <hcbr>
		fld   dword [data + SYN.hcbufl]   ; <hcbl> <lcbr> <hcbr>
		fld   dword [data + SYN.lcbufl]   ; <lcbl> <hcbl> <lcbr> <hcbr>
.lhcloop
			; low cut
			fld			st0                       ; <lcbl> <lcbl> <hcbl> <lcbr> <hcbr>
			fsubr		dword [esi]								; <in-l=lcoutl> <lcbl> <hcbl> <lcbr> <hcbr>
			fld			st0                       ; <in-l> <lcoutl> <lcbl> <hcbl> <lcbr> <hcbr>
			fmul		dword [data + SYN.lcfreq] ; <f*(in-l)> <lcoutl> <lcbl> <hcbl> <lcbr> <hcbr>
			faddp		st2, st0                  ; <lcoutl> <lcbl'> <hcbl> <lcbr> <hcbr>
			fld     st3                       ; <lcbr> <lcoutl> <lcbl'> <hcbl> <lcbr> <hcbr>
			fsubr		dword [esi+4]							; <in-l=lcoutr> <lcoutl> <lcbl'> <hcbl> <lcbr> <hcbr>
			fld     st0                       ; <in-l> <lcoutr> <lcoutl> <lcbl'> <hcbl> <lcbr> <hcbr>
			fmul		dword [data + SYN.lcfreq] ; <f*(in-l)> <lcoutr> <lcoutl> <lcbl'> <hcbl> <lcbr> <hcbr>
			faddp   st5, st0                  ; <lcoutr> <lcoutl> <lcbl'> <hcbl> <lcbr'> <hcbr>
			fxch    st1                       ; <lcoutl> <lcoutr> <lcbl'> <hcbl> <lcbr'> <hcbr>
			cmp     eax, 3f800000h            
			je      .nohighcut
			; high cut
			fld     st3                       ; <hcbl> <lcoutl> <lcoutr> <lcbl'> <hcbl> <lcbr'> <hcbr>
			fsubp   st1, st0                  ; <inl-hcbl> <lcoutr> <lcbl'> <hcbl> <lcbr'> <hcbr>
			fld     st5												; <hcbr> <inl-hcbl> <lcoutr> <lcbl'> <hcbl> <lcbr'> <hcbr>
			fsubp   st2, st0                  ; <inl-hcbl> <inr-hcbr> <lcbl'> <hcbl> <lcbr'> <hcbr>
			fmul    dword [data + SYN.hcfreq] ; <f*(inl-l)> <inr-hcbr> <lcbl'> <hcbl> <lcbr'> <hcbr>
			fxch    st1                       ; <inr-hcbr> <f*(inl-l)> <lcbl'> <hcbl> <lcbr'> <hcbr>
			fmul    dword [data + SYN.hcfreq] ; <f*(inr-l)> <f*(inl-l)> <lcbl'> <hcbl> <lcbr'> <hcbr>
			fxch    st1                       ; <f*(inl-l)> <f*(inr-l)> <lcbl'> <hcbl> <lcbr'> <hcbr>
			faddp   st3, st0                  ; <f*(inr-l)> <lcbl'> <hcbl'> <lcbr'> <hcbr>
			faddp   st4, st0                  ; <lcbl'> <hcbl'> <lcbr'> <hcbr'>
			fld     st3                       ; <hcoutr> <lcbl'> <hcbl'> <lcbr'> <hcbr'>
			fld     st2                       ; <hcoutl> <hcoutr> <lcbl'> <hcbl'> <lcbr'> <hcbr'>
.nohighcut
			fstp    dword [esi]               ; <outr> <lcbl'> <hcbl> <lcbr'> <hcbr>
			fstp    dword [esi+4]             ; <lcbl'> <hcbl> <lcbr'> <hcbr>
			lea			esi, [esi+8]

			dec ecx
		jnz		.lhcloop
		
		fstp  dword [data + SYN.lcbufl]   ; <hcbl> <lcbr> <hcbr>
		fstp  dword [data + SYN.hcbufl]   ; <lcbr> <hcbr>
		fstp  dword [data + SYN.lcbufr]   ; <hcbr>
		fstp  dword [data + SYN.hcbufr]   ; -

    ; Kompressor
		mov		ecx, [todo+4]
		mov		esi, mixbuf
		mov		edi, mixbuf
		lea		ebp, [data + SYN.compr]
    call  syCompRender

.renderend
%ifdef VUMETER
		mov		ecx, [todo+4]
		mov   esi, mixbuf
		lea   edi, [data + SYN.mainvu]
		call  .vumeter
%endif	

    V2PerfLeave V2Perf_TOTAL
    V2PerfCopy	  
		popad
		ret



%ifdef VUMETER
.vumeter
      ; esi: srcbuf, ecx: len, edi: &outv
      pushad
      mov   [temp], ecx
		  mov		eax, [data + SYN.vumode]		  
		  jnz		.vurms
		  
		  ; peak vu meter
		  .vploop
		    mov		eax, [esi]
		    and   eax, 0x7fffffff
		    cmp   eax, [edi]
		    jbe   .vplbe1
		    mov   [edi], eax
		    .vplbe1
		    mov		eax, [esi+4]
		    and   eax, 0x7fffffff
		    cmp   eax, [edi+4]
		    jbe   .vplbe2
		    mov   [edi+4], eax
		    .vplbe2
		    fld   dword [edi]
		    fmul  dword [fcvufalloff]
		    fstp  dword [edi]
		    fld   dword [edi+4]
		    fmul  dword [fcvufalloff]
		    fstp  dword [edi+4]
		    lea   esi,  [esi+8]
		    dec ecx
		    jnz .vploop
				popad
				ret

.vurms		  
			  ; rms vu meter			 
		    fldz											; <lsum>
		    fldz											; <rsum> <lsum>
		    .vrloop
		      fld     dword [esi]     ; <l> <rsum> <lsum>
		      fmul    st0, st0        ; <l²> <rsum> <lsum>
		      faddp   st2, st0        ; <rsum> <lsum'>
		      fld     dword [esi+4]   ; <r> <rsum> <lsum>
			    lea			esi, [esi+8]
		      fmul    st0, st0        ; <r²> <rsum> <lsum>
		      faddp   st1, st0        ; <rsum'> <lsum'>
		      dec     ecx
		    jnz .vrloop               ; <rsum> <lsum>
		    fld1                      ; <1> <rsum> <lsum>
		    fidiv  dword [temp]       ; <1/fl> <rsum> <lsum>
		    fmul   st2, st0           ; <1/fl> <rsum> <lout>
		    fmulp  st1, st0           ; <rout> <lout>
		    fxch   st1                ; <lout> <rout>
		    fstp   dword [edi] ; <rout>
		    fstp  dword  [edi+4] ; -		  				  
				popad
				ret
%endif		  





; ------------------------------------------------------------------------
; PROCESS MIDI MESSAGES

global _MIDI_
_MIDI_

global _synthProcessMIDI@4


spMTab dd ProcessNoteOff
			 dd ProcessNoteOn
			 dd ProcessPolyPressure
			 dd ProcessControlChange
			 dd ProcessProgramChange
			 dd ProcessChannelPressure
			 dd ProcessPitchBend
			 dd ProcessRealTime


_synthProcessMIDI@4:
  pushad

	fstcw	[oldfpcw]
	mov		ax, [oldfpcw]
	and		ax, 0f0ffh
	or		ax, 3fh
	mov		[temp], ax
	finit
	fldcw	[temp]

	mov		esi, [esp + 36]
.mainloop
		xor		eax, eax	
		mov		al, [esi]
		test	al, 80h
		jz    .nocmd
		cmp   al, 0fdh  ; end of stream ?
		jne   .noend
.end
    fldcw [oldfpcw]
		popad
		ret 4
.noend
    mov   [data + SYN.mrstat], eax ; update running status
		inc   esi
		mov		al, [esi]
.nocmd
		mov   ebx, [data + SYN.mrstat]
		mov   ecx, ebx
		and   cl, 0fh
		shr   ebx, 4
		test  bl, 8
		jz    .end
		and   ebx, 07h
		call  [spMTab + 4*ebx]
	jmp .mainloop

; esi: ptr auf midistream (muß nachher richtig sein!)
; ecx: channel aus commandbyte


; ------------------------------------------------------------------------
; Note Off


ProcessNoteOff:
	movzx edi, byte [esi]  ; Note
	; Nach Voice suchen, die die Note spielt
	xor		edx, edx

%ifdef RONAN
	cmp		cl, 15 ; Speechsynth-Channel?
	jne		.srvloop
	pushad
	call  _ronanCBNoteOff@0
	popad
%endif
	
.srvloop
    cmp   ecx, [data + SYN.chanmap+4*edx] ; richtiger channel?
		jne		.srvlno
		mov		eax, edx
		imul	eax, syWV2.size
		lea   ebp, [data + SYN.voicesw + eax]
		cmp   edi, [ebp + syWV2.note]
		jne   .srvlno
		test  byte [ebp + syWV2.gate], 255
		jz    .srvlno

		call  syV2NoteOff
		jmp   .end
.srvlno
    inc   edx
		cmp   dl, POLY
	jne .srvloop
		  
.end
  add esi, byte 2
	ret



; ------------------------------------------------------------------------
; Note On

ProcessNoteOn:
  ; auf noteoff checken
	mov al, [esi+1]
	or  al, al
	jnz .isnoteon
	jmp ProcessNoteOff
.isnoteon

%ifdef RONAN
	cmp		cl, 15 ; Speechsynth-Channel?
	jne		.noronan
	pushad
	call  _ronanCBNoteOn@0
	popad
.noronan
%endif

	movzx eax, byte [data + SYN.chans + 8*ecx]   ; pgmnummer
	mov   ebp, [data + SYN.patchmap]
	mov		ebp, [ebp + 4*eax]           ; sounddef
	add		ebp, [data + SYN.patchmap]

	; maximale polyphonie erreicht?
	xor ebx, ebx
	mov edx, POLY
.chmploop
    mov eax, [data + SYN.chanmap+4*edx - 4]
		cmp eax, ecx
		jne .notthischan
		inc ebx
.notthischan
    dec edx
	jnz .chmploop

	cmp bl,  byte [ebp + v2sound.maxpoly] ; poly erreicht?
	jae .killvoice	; ja -> alte voice töten

	; freie voice suchen
	xor edx, edx
.sfvloop1
    mov	eax, [data + SYN.chanmap+4*edx]
		or	eax, eax
		js	.foundfv
		inc edx
		cmp dl, POLY
	jne .sfvloop1

	; keine freie? gut, dann die älteste mit gate aus
	mov	edi, [data + SYN.curalloc]
	xor	ebx, ebx
	xor edx, edx
	not edx
.sfvloop2
		mov		eax, ebx
		imul	eax, syWV2.size
		mov   eax, [data + SYN.voicesw + eax + syWV2.gate]
		or    eax, eax
		jnz   .sfvl2no
		cmp		edi, [data + SYN.allocpos+4*ebx]
		jbe		.sfvl2no
		mov		edi, [data + SYN.allocpos+4*ebx]
		mov		edx, ebx
.sfvl2no
		inc ebx
		cmp bl, POLY
	jne .sfvloop2

	or	edx, edx
	jns	.foundfv

	; immer noch keine freie? gut, dann die ganz älteste
	mov eax, [data + SYN.curalloc]
	xor	ebx, ebx
.sfvloop3
		cmp eax, [data + SYN.allocpos+4*ebx]
		jbe .sfvl3no
		mov eax, [data + SYN.allocpos+4*ebx]
		mov edx, ebx
.sfvl3no
		inc ebx
		cmp bl, POLY
	jne .sfvloop3
	; ok... die nehmen wir jezz.
.foundfv
	jmp .donoteon

.killvoice		
  ; älteste voice dieses channels suchen
	mov	edi, [data + SYN.curalloc]
	xor	ebx, ebx
	xor edx, edx
	not edx
.skvloop2
		cmp		ecx, [data + SYN.chanmap+4*ebx]
		jne		.skvl2no
		mov		eax, ebx
		imul	eax, syWV2.size
		mov   eax, [data + SYN.voicesw + eax + syWV2.gate]
		or    eax, eax
		jnz   .skvl2no
		cmp		edi, [data + SYN.allocpos+4*ebx]
		jbe		.skvl2no
		mov		edi, [data + SYN.allocpos+4*ebx]
		mov		edx, ebx
.skvl2no
		inc ebx
		cmp bl, POLY
	jne .skvloop2

	or	edx, edx
	jns	.donoteon

  ; nein? -> älteste voice suchen
	mov eax, [data + SYN.curalloc]
	xor ebx, ebx
.skvloop
		cmp ecx, [data + SYN.chanmap+4*ebx]
		jne .skvlno
		cmp eax, [data + SYN.allocpos+4*ebx]
		jbe .skvlno
		mov eax, [data + SYN.allocpos+4*ebx]
		mov edx, ebx
.skvlno
    inc ebx
		cmp bl, POLY
	jne .skvloop
	; ... und damit haben wir die voice, die fliegt.

.donoteon
	; (ecx immer noch chan, edx voice, ebp ptr auf sound)
	mov [data + SYN.chanmap  + 4*edx], ecx  ; channel
	mov [data + SYN.voicemap + 4*ecx], edx  ; current voice
	mov eax, [data + SYN.curalloc]
	mov [data + SYN.allocpos + 4*edx], eax  ; allocpos
	inc dword [data + SYN.curalloc]

	call		storeV2Values ; Values der Voice updaten  
	mov			eax, edx
	imul		eax, syWV2.size
	lea			ebp, [data + SYN.voicesw + eax]
	movzx		eax, byte [esi]    ; Note
	movzx		ebx, byte [esi+1]  ; Velo

	call		syV2NoteOn
  add			esi, byte 2
	ret

; ------------------------------------------------------------------------
; Aftertouch


ProcessChannelPressure:
  add esi, byte 1
	ret


; ------------------------------------------------------------------------
; Control Change


ProcessControlChange:
  movzx eax, byte [esi]
	or		eax, eax        ; und auf 1-7 checken
	jz    .end
	cmp   al, 7
%ifdef FULLMIDI
  ja    .chanmode
%else
	ja    .end
%endif

	lea		edi, [data + SYN.chans + 8*ecx]
	movzx ebx, byte [esi+1]
	mov   [edi+eax], bl
	
	cmp   cl, 15 ; sprachsynth?
	jne   .end
%ifdef RONAN
	pushad
	push  ebx
	push  eax
	call  _ronanCBSetCtl@8
	popad
%endif

	
.end
  add esi,2
	ret
	
%ifdef FULLMIDI	
.chanmode
  cmp al, 120
  jb  .end
  ja  .noalloff
  
	; CC #120 : All Sound Off
	xor edx, edx
	mov ebp, data + SYN.voicesw
.siloop1
    cmp   byte [data + SYN.chanmap - 4 + 4*edx], cl
    jnz   .noreset
    call	syV2Init
		mov   dword [data + SYN.chanmap - 4 + 4*edx], -1
.noreset
		lea		ebp, [ebp+syWV2.size]
		inc   dl
		cmp   dl, POLY
		jnz .siloop1	
	
	jmp short .end
  
  
.noalloff
  cmp al, 121
  ja  .noresetcc
  ; CC #121 : Reset All Controllers
  ; evtl TODO, who knows

	jmp short .end
  
.noresetcc
  cmp al, 123
  jb  .end
  ; CC #123 : All Notes Off
  ; and CC #124-#127 - channel mode changes (ignored)

%ifdef RONAN
	cmp		cl, 15 ; Speechsynth-Channel?
	jne		.noronanoff
	pushad
	call  _ronanCBNoteOff@0
	popad
.noronanoff
%endif
	
	xor edx, edx
	mov ebp, data + SYN.voicesw
.noffloop1
    cmp   byte [data + SYN.chanmap - 4 + 4*edx], cl
    jnz   .nonoff
		call  syV2NoteOff
.nonoff
		lea		ebp, [ebp+syWV2.size]
		inc   dl
		cmp   dl, POLY
	jnz .noffloop1
	
	jmp short .end  

%endif
  


; ------------------------------------------------------------------------
; Program change


ProcessProgramChange:
  ; check ob selbes program
	lea		edi, [data + SYN.chans + 8*ecx]
  mov   al, [esi]
  and   al, 7fh
  cmp   al, [edi]
  jz    .sameprg
	mov   [edi], al
  
	; ok, alle voices, die was mit dem channel zu tun haben... NOT-AUS!
  xor edx, edx
	xor eax, eax
	not eax
.notausloop
	  cmp ecx, [data + SYN.chanmap + 4*edx]
		jnz .nichtaus
		mov [data + SYN.chanmap + 4*edx], eax
.nichtaus
		inc edx
		cmp dl, POLY
	jne .notausloop

	; pgm setzen und controller resetten
.sameprg
	mov   cl, 6
  inc		esi
	inc		edi
	xor   eax, eax
	rep		stosb
	ret

; ------------------------------------------------------------------------
; Pitch Bend

ProcessPitchBend:
  add esi, byte 2
	ret


; ------------------------------------------------------------------------
; Poly Aftertouch

ProcessPolyPressure:   ; unsupported
  add esi, byte 2
	ret


; ------------------------------------------------------------------------
; Realtime/System messages


ProcessRealTime:
%ifdef FULLMIDI
	cmp cl, 0fh
	jne .noreset
	
	pushad
	push dword [data + SYN.samplerate]
	push dword [data + SYN.patchmap]
	call _synthInit@8
	popad

.noreset
%endif
	ret


; ------------------------------------------------------------------------
; Noch wichtiger.


global _synthSetGlobals@4
_synthSetGlobals@4
	pushad
	xor   eax, eax
	xor   ecx, ecx
	mov   esi, [esp+36]
	lea   edi, [data + SYN.rvbparm]
  mov   cl,  (SYN.globalsend-SYN.globalsstart)/4
.cvloop
		lodsb
		mov   [temp], eax
		fild  dword [temp]
		fstp  dword [edi]
		lea   edi, [edi+4]
		dec   ecx
	jnz .cvloop

	lea   esi, [data + SYN.rvbparm]
	lea   ebp, [data + SYN.reverb]
	call	syReverbSet
	lea   esi, [data + SYN.delparm]
	lea   ebp, [data + SYN.delay]
	call  syModDelSet
  lea   esi, [data + SYN.cprparm]
  lea   ebp, [data + SYN.compr]
  call  syCompSet

	fld   dword [data + SYN.vlowcut]
	fld1
	faddp st1, st0
	fmul  dword [fci128]
	fmul  st0, st0
	fstp  dword [data + SYN.lcfreq]
	fld   dword [data + SYN.vhighcut]
	fld1
	faddp st1, st0
	fmul  dword [fci128]
	fmul  st0, st0
	fstp  dword [data + SYN.hcfreq]

	popad
	ret	4


; ------------------------------------------------------------------------
; VU retrieval

%ifdef VUMETER

global _synthSetVUMode@4
_synthSetVUMode@4:
  mov eax, [esp + 4]
  mov [data + SYN.vumode], eax
  ret 4
  

global _synthGetChannelVU@12
_synthGetChannelVU@12:
  pushad
  mov  ecx, [esp + 36]
  mov  esi, [esp + 40]
  mov  edi, [esp + 44]
  mov  eax, [data + SYN.chanvu + 8*ecx]
  mov  [esi], eax
  mov  eax, [data + SYN.chanvu + 8*ecx + 4]
  mov  [edi], eax
  popad
  ret 12

global _synthGetMainVU@8
_synthGetChannelVU@8:
  pushad
  mov  esi, [esp + 36]
  mov  edi, [esp + 40]
  mov  eax, [data + SYN.mainvu]
  mov  [esi], eax
  mov  eax, [data + SYN.mainvu+4]
  mov  [edi], eax
  popad
  ret 8



%endif


; ------------------------------------------------------------------------
; Debugkram

global _synthGetPoly@4

_synthGetPoly@4:
  pushad
	mov ecx, 17
	mov edi, [esp + 36]
	xor eax, eax
	rep stosd

	mov edi, [esp + 36]
.gploop
		mov ebx, [data + SYN.chanmap + 4*eax]
		or  ebx, ebx
		js  .gplend
		inc dword [edi + 4*ebx]
		inc dword [edi + 4*16]
.gplend
    inc eax
		cmp al, POLY
	jne .gploop

	popad
	ret 4


global _synthGetPgm@4

_synthGetPgm@4:
  pushad
	mov edi, [esp + 36]
	xor ecx, ecx

.gploop
		movzx eax, byte [data + SYN.chans + 8*ecx]
		stosd
    inc ecx
		cmp cl, 16
	jne .gploop

	popad
	ret 4


global _synthGetFrameSize@0
_synthGetFrameSize@0:
  mov eax, [SRcFrameSize]
  ret
