; This code is in the public domain. See LICENSE for details.

; runtime library

bits        32

section     .data

%if 0
einhalb     dd 0.5
halfpi      dd 1.5707963268
asinconsts  dd 0.892399, 1.693204, -3.853735, 2.838933
%endif

%define DATAMAXSIZE   256*1024

global      _mainDialog
_mainDialog incbin "configdlg.bin"

global      _introData
_introData  db "{data insert mark here}"
            dd DATAMAXSIZE
%ifdef DEBUG
            db 1
%else
            db 0
%endif
            resb (_introData+DATAMAXSIZE-$)

%if 0
global      _loaderTune
_loaderTune incbin "loading.v2m"
%endif

section		  .drectve info
	db "/merge:.CRT=.data"

section     .CRT$XCA data align=4
global      __xc_a                     ; c++ init
__xc_a:
section     .CRT$XCZ data align=4
global      __xc_z
__xc_z:

section     .text

; ---- void setupFPU()

global      _setupFPU@0
_setupFPU@0:
    push    dword 0x1e3f
    fldcw   [esp]
    pop     eax
	cld
    ret

%if 0
; ---- sF32 sin(sF32 a)

global      _sin@4
_sin@4:
	fld		dword [esp+4]
	fsin
	ret		4

; ---- sF32 cos(sF32 a)

global      _cos@4
_cos@4:
	fld		dword [esp+4]
	fcos
	ret		4
%endif

; ---- sF32 asin(sF32 a) // assumes correct range

%if 0
global      _asin
_asin:
    fld     qword [esp+4]
asingo:
    fld     st0
    fmul    st0             ; x² x
    mov     eax, asinconsts
    fld     dword [eax+12]
    fmul    st1
    fadd    dword [eax+8]
    fmul    st1
    fadd    dword [eax+4]
    fmulp   st1, st0
    fadd    dword [eax]
    fmulp   st1, st0
    ret

; ---- sF32 acos(sF32 a) // assumes correct range

global      _acos
_acos:
    fld     qword [esp+4]
    call    asingo
    fld     dword [halfpi]
    fsubrp  st1, st0
    ret
%endif

%if 0
; ---- sF32 atan2(sF32 y, sF32 x)

global      _atan2@8
_atan2@8:
	fld		dword [esp+4]
	fld		dword [esp+8]
	fpatan
	ret		8

; ---- sF32 sqrt(sF32 a)

global      _sqrt@4
_sqrt@4:
	fld		dword [esp+4]
	fsqrt
	ret		4
%endif

; ---- sF32 pow(sF32 a, sF32 b)

global    _pow
_pow:
  fld     qword [esp+4]
  fld     qword [esp+12]
  call    __CIpow

global    __CIpow
__CIpow:
  fxch    st1
	ftst
	fstsw	ax
	sahf
	jz		  .zero

	fyl2x
	fld1
	fld		  st1
	fprem
	f2xm1
	faddp	  st1, st0
	fscale

.zero:
	fstp	  st1
	ret

; ---- sF32 exp(sF32 a)

%if 0
global      _exp
_exp:
    fld     qword [esp+4]
%endif

global      __CIexp
__CIexp:
    fldl2e
    fmulp   st1, st0
    fld1
    fld     st1
    fprem
    f2xm1
    faddp   st1, st0
    fscale
    fstp    st1
    ret

; ---- sF32 ceil(sF32 a)

global      _ceil
_ceil:
    fld     qword [esp+4]
    frndint
    fcom    qword [esp+4]
    fstsw   ax
    sahf
    je      .equal
    fld1
    faddp   st1, st0
.equal:
    ret

; ---- sF32 floor(sF32 a)

global      _floor
_floor:
    fld     qword [esp+4]
    frndint
    fcom    qword [esp+4]
    fstsw   ax
    sahf
    je      .equal
    fld1
    fsubp   st1, st0
.equal:
    ret

; ---- void * __cdecl memcpy(void *d, const void *s, sInt c)

global      _memcpy
_memcpy:
	push	esi
	push	edi

	lea		ecx, [esp+12]
	mov		edi, [ecx]
	mov		esi, [ecx+4]
	mov		ecx, [ecx+8]
	mov		eax, ecx
	and		eax, byte 7
	push	eax

	shr		ecx, 3
	lea		esi, [esi+ecx*8]
	lea		edi, [edi+ecx*8]
	neg		ecx

.cplp:
	mov		eax, [esi+ecx*8]
	mov		edx, [esi+ecx*8+4]
	mov		[edi+ecx*8], eax
	mov		[edi+ecx*8+4], edx
	inc		ecx
	jnz		.cplp

	pop		ecx
    cld
	rep		movsb

	pop		edi
	pop		esi
  mov     eax, edi
  ret

; ---- void * __cdecl memset(void *d, char c, int count)

global _memset
_memset:
	push	edi
	mov		edi, [esp+8]
	mov		eax, [esp+12]
	mov		ecx, [esp+16]
	mov		ah, al
	mov		edx, eax
	shl		eax, 16
	mov		ax, dx
	push	ecx
	shr		ecx, 2
	jz		.tail
	rep		stosd
.tail:
	pop		ecx
	and		ecx, byte 3
	jz		.done
	rep		stosb
.done:
	pop		edi
	ret

; ---- void memzero(void *d, sInt c)

%if 0
global      _memzero@8
_memzero@8:
	push	edi
	mov		edi, [esp+8]
	mov		ecx, [esp+12]
	xor		eax, eax
	cdq
	shr		ecx, 1
	rcl		edx, 1
	shr		ecx, 1
	rcl		edx, 1
    cld
	rep		stosd
	xchg	ecx, edx
	rep		stosb
	pop		edi
	ret		8

; ---- void memfilld(void *d, sU32 v, sInt c)

global      _memfilld@12
_memfilld@12:
	push	ebp
	push	edi
	lea		ebp, [esp+12]
	mov		edi, [ebp]
	mov		eax, [ebp+4]
	mov		ecx, [ebp+8]
  cld
	rep		stosd
	pop		edi
	mov		eax, edi
	pop		ebp
	ret		12
%endif

; ---- size_t __cdecl strlen(const char *s)

global      _strlen
_strlen:
	mov		edx, [esp+4]
	xor		eax, eax

.lp:
	cmp		byte [edx+eax], 0
	jz		.end

	inc		eax
	jmp		short .lp

.end:
    ret

%ifndef DEBUG

global    __fltused
__fltused:

%define PAGESIZE 0x1000

global    __chkstk ; wenn das jetzt geht, dann kotze ich.
__chkstk:
  cmp	    eax, PAGESIZE
  jae		short .probesetup

  neg		eax
  add		eax, esp
  add		eax, byte 4
  test		dword [eax], eax
  xchg		eax, esp
  mov		eax, dword [eax]
  push		eax
  ret

.probesetup:
  push		ecx
  lea		ecx, [esp+8]

.probepages:
  sub		ecx, PAGESIZE
  sub		eax, PAGESIZE
 
  test		dword [ecx], eax

  cmp		eax, PAGESIZE
  jae		short .probepages

  sub		ecx, eax
  mov		eax, esp

  test		dword [ecx], eax

  mov		esp, ecx
  
  mov		ecx, dword [eax]
  mov		eax, dword [eax+4]
  
  push		eax
  ret

%endif

%if 0 ; deprecated code follows ------------------------------------------------------------------------------------------------

; ---- float->int

global		__ftol
__ftol:
	sub		esp, byte 8
	fistp	qword [esp]
	pop		eax
	pop		edx
	ret

; ---- sF32 asin(sF32 a)

global      _asin@4
_asin@4:
	fld		dword [esp+4]
	fld		st0
	fabs
	fcom	dword [einhalb]
	fstsw	ax
	sahf
	jbe		.kleiner
	fld1
	fsubrp	st1, st0
	fld		st0
	fadd	st0
	fxch	st1
	fmul	st0
	fsubp	st1, st0
	jmp		short .end

.kleiner:
	fstp	st0
	fld		st0
	fmul	st0
	fld1
	fsubrp	st1, st0
	
.end:
	fsqrt
	fpatan
	ret		4

; ---- sF32 acos(sF32 a)

global      _acos@4
_acos@4:
	fld		dword [esp+4]
	fld1
	fchs
	fcomp	st1
	fstsw	ax
	je		.suckt

	fld		st0
	fld1
	fsubrp	st1, st0
	fxch	st1
	fld1
	faddp	st1, st0
	fdivp	st1, st0
	fsqrt
	fld1
	jmp		short .end

.suckt:
	fld1
	fldz

.end:
	fpatan
	fadd	st0, st0
	ret		4
%endif

