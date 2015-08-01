.CODE
get_bp_sp_ip proc
	
	mov [rcx], rbp
	lea rcx, [rsp+8]
	mov [rdx], rcx
	mov rcx, [rsp]
	mov [r8], rcx
	
	ret
get_bp_sp_ip endp

END