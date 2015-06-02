.386
.model flat
option casemap :none 

.CODE
@u64div32@16 proc

	mov eax,dword ptr [esp+4]
	test edx,0FFFFFFFFh
	jne lp
	mov edx,dword ptr [esp+8]
	div ecx
	ret 8
lp:
	push edx
	mov edx,dword ptr [esp+12]
	div ecx
	pop ecx
	mov dword ptr [ecx],edx
	ret 8

@u64div32@16 endp

END