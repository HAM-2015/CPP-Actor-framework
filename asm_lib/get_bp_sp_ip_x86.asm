.386
.model flat
option casemap :none 

.CODE
@get_bp_sp_ip@12 proc
	
	mov [ecx], ebp
	lea ecx, [esp+8]
	mov [edx], ecx
	mov edx, [esp+4]
	mov ecx, [esp]
	mov [edx], ecx
	
	ret 4
@get_bp_sp_ip@12 endp

END