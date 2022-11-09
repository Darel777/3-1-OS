section .data
	;27 means \033;[1;31m表示红色高亮;[31m表示红色
	red:	 	db  27, '[31m'
    	.len:   equ $ - red
	;恢复默认，清空所有特殊模式
	ori: 		db 27, '[0m'
    	.len:   equ $ - ori

section .text
	global my_print
	global back_to_default
	global change_to_red

	;my_print(char* c, int length);
	;利用栈传递参数
	my_print:
		mov	eax,4
		mov	ebx,1
		mov	ecx,[esp+4];char * c
		mov	edx,[esp+8];length 
		int	80h
		ret
		
	
	back_to_default:
		mov eax, 4
		mov ebx, 1
		mov ecx, ori
		mov edx, ori.len
		int 80h
		ret

	change_to_red:
		mov eax, 4
		mov ebx, 1
		mov ecx, red
		mov edx, red.len
		int 80h
		ret