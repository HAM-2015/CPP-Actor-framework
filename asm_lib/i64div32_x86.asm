.386
.model flat
option casemap :none 

.CODE
@i64div32@16 proc

	mov eax,dword ptr [esp+4]
	test edx,0FFFFFFFFh
	jne lp
	mov edx,dword ptr [esp+8]
	idiv ecx
	ret 8
lp:
	push edx
	mov edx,dword ptr [esp+12]
	idiv ecx
	pop ecx
	mov dword ptr [ecx],edx
	ret 8

@i64div32@16 endp

END