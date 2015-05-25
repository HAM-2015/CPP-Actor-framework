.CODE
cpu_tick proc
	push	rdx
	db		0Fh
	db		31h
	shl		rdx, 32
	or		rax, rdx
	pop		rdx
	ret
cpu_tick endp

END