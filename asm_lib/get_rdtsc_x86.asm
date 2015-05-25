.386
.model flat
option casemap :none 

.CODE
@cpu_tick@0 proc
	
	db 0Fh
	db 31h
	
	ret
@cpu_tick@0 endp

END