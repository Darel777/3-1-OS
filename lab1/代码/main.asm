SECTION .data 
message_input db "Please input x , y and operator: ", 0h;0是数字 h是十六进制 0h代表\0 
message_sumoutput db "The sum is: ", 0h ;TODO
message_productoutput db "The product is: ", 0h ;TODO 
message_invalid db "Invalid input.", 0h 
line: db 0Ah 

SECTION .bss 
input_number: resb 255;接收输入的数字
first_number: resb 255;被拆分的第一个数字
first_len: resb 255;第一个数字的长度
second_number: resb 255;被拆分的第二个数字
second_len: resb 255;第二个数字的长度
operator: resb 1;符号
sum: resb 255;和
product: resb 255;积

SECTION .text 
global main
;main函数
main: 
    .start:
        ;提示输入并对输入进行处理
        mov eax, message_input
        call print_eax
        call getline
        call split_and_check
        ;对正确的输入进行加法和乘法的判断
        cmp BYTE[operator], '+'
        je .jia
        jmp .cheng
        ;加法乘法进程
        .jia:
            call jiafa
            mov eax, sum
            jmp .after_cal
        .cheng:
            call chengfa
            mov eax, product
            jmp .after_cal
        ;后处理
        .after_cal:
            call format
            call print
            call newline
            jmp .fin
        ;循环
        .fin:
            call clear
            jmp .start
clearone:;清理长度为255的段
    push eax
    push ebx
    mov ebx, 255
    .loop:
        mov byte[eax], 0
        inc eax
        dec ebx
        jnz .loop   
    pop ebx
    pop eax
    ret
clearoperator:;清理操作数
    push eax
    push ebx
    mov ebx, 1;
    .loop:
        mov byte[eax], 0
        inc eax
        dec ebx
        jnz .loop   
    pop ebx
    pop eax
    ret            
printnumber:;让计算的数字变成打印的数字 加48
    push eax
    push ecx
    push edx
    push esi
    mov ecx, 0
    .Loop1:
        inc ecx       
        mov edx, 0
        mov esi, 10
        idiv esi     
        add edx, 48   
        push edx       
        cmp eax, 0
        jnz .Loop1
    .Loop2:
        dec ecx
        mov eax, esp
        call print_eax
        pop eax
        cmp ecx, 0
        jnz .Loop2
        pop esi
        pop edx
        pop ecx
        pop eax
        ret 
invalid:;输入不合法
    mov eax, message_invalid
    call clear
    call print_eax
    call newline
    call main
strlen:;将eax地址字符长度赋给eax
    push ebx
    mov ebx, eax
    .next:
        cmp byte[eax], 0
        jz .finish
        inc eax
        jmp .next
    .finish:
        sub eax, ebx
    pop ebx
    ret
print_eax:;打印eax地址中存储的内容
    cmp BYTE[eax], 0
    jz .print_eax_ret 
    push edx
    push ecx
    push ebx
    push eax 
    call strlen
    mov edx, eax 
    pop eax
    mov ecx, eax 
    mov ebx, 1 
    mov eax, 4 
    int 80h
    mov eax, ecx 
    pop ebx
    pop ecx
    pop edx
    .print_eax_ret:
        ret
getline:;获取一行输入
    push edx
    push ecx
    push ebx
    push eax
    mov edx, 255 
    mov ecx, input_number 
    mov ebx, 0 
    mov eax, 3 
    int 80h 
    pop eax
    pop ebx
    pop ecx
    pop edx
    ret
split_and_check:;对输入进行分析并判断是否不正确，如果正确储存
    mov ecx, input_number
    mov eax, first_number
    mov ebx, second_number
    mov esi, first_len
    mov edi, second_len
    .loop_first:
        cmp byte[ecx], 32 
        jz .pre_ret1
        cmp byte[ecx], 10 
        jz .pre_ret1
        cmp byte[ecx], 42 
        jz .pre_ret2
        cmp byte[ecx], 43 
        jz .pre_ret2
        cmp byte[ecx], 113
        jz .pre_ret3
        mov dl, byte[ecx] 
        mov byte[eax], dl 
        inc eax           
        inc byte[esi]
        inc ecx           
        jmp .loop_first
    .loop_second:
        cmp byte[ecx], 10
        jz .pre_ret
        mov dl, byte[ecx] 
        mov byte[ebx], dl 
        inc ebx           
        inc byte[edi]
        inc ecx           
        jmp .loop_second  
    .pre_ret:
        mov dl, byte[ecx]
        mov byte[ebx], dl
        ret
    .pre_ret1:
        call invalid
    .pre_ret2:
        push eax
        push ebx
        push edx
        mov eax, operator
        mov dl, byte[ecx]
        mov byte[eax], dl
        pop edx
        pop ebx
        pop eax
        inc ecx
        jmp .loop_second
    .pre_ret3:
        mov ebx, 0
        mov eax, 1
        int 0x80
jiafa:;加法流程
   	push eax
	push ebx
	push edx
   	mov ecx, sum
   	mov eax, first_number
   	mov ebx, second_number
  	mov esi, dword[first_len]
   	mov edi, dword[second_len]
   	cmp esi, edi;比较长度 把长的那个多出的几位先放进去
   	ja .fill_1
   	je .add_end
   	jmp .fill_2
   	.fill_1:
    	sub esi, edi
   	.loop1:
      	cmp esi, 0
      	je .add_end
      	mov dl, byte[eax]
  	   	mov byte[ecx], dl
  	   	inc eax
  	   	inc ecx
  	   	dec esi
      	jmp .loop1
   	.fill_2:
      	sub edi, esi
   	.loop2:
      	cmp edi, 0
  	   	je .add_end
		mov dl, byte[ebx]
		mov byte[ecx], dl
		inc ebx
		inc ecx
		dec edi
		jmp .loop2
   .add_end:;末尾加法
      	cmp byte[eax], 0
  	   	je .finish
  	   	mov dl, byte[eax]
  	   	add dl, byte[ebx]
      	sub dl, 30h
  	   	mov byte[ecx], dl
  	   	inc eax
  	   	inc ebx
  	   	inc ecx
      	jmp .add_end
   .finish:
      	pop edx
		pop ebx
		pop eax
        push eax
        mov eax, message_sumoutput
        call print_eax
        pop eax
      	ret
chengfa:;乘法流程
    pushad
    mov ecx, product 
    mov edx, first_number
    mov ebx, second_number
    mov esi, 0
    mov edi, 0
   	mov eax, 0
   	add eax, dword[first_len]
   	add eax, dword[second_len];生成的长度
	.init:;初始化 让该有的位数变成0
        cmp eax,0
        je .loop1
        mov byte[ecx],'0'
        dec eax
        inc ecx
        jmp .init
  	.loop1:;第一个数置尾
        cmp byte[edx+1],0
        je .loop2
        inc edx
        jmp .loop1
  	.loop2:;第二个数置尾
        cmp byte[ebx+2],0
        je .multi
        inc ebx
        jmp .loop2
  	.multi:
        push ebx
        mov eax, 0
        cmp edx, first_number
        jb .finish_multi
        mov al, byte[edx]  
        sub al, 30h
        jmp .mul_loop
  	.mark:
        dec edx
        inc esi
        jmp .multi
  	.mul_loop:
        cmp ebx, second_number
        jb .finish_mulLoop
        mov ah, byte[ebx]
        push edx
        mov dl, al
        sub ah, 30h
        mul ah
        push ecx
        sub ecx, esi
        sub ecx, edi
        sub ecx, 1
        cmp byte[ecx], 150
        ja .simple_format_byte
  	.finish_simple_format_byte:
        add byte[ecx], al
        pop ecx
        mov al, dl
        pop edx
        dec ebx
        inc edi
        jmp .mul_loop
  	.finish_mulLoop:
        mov edi, 0
        pop ebx
        jmp .mark
  	.finish_multi:
        pop ebx
        popad
        push eax
        mov eax, message_productoutput
        call print_eax
        pop eax
        ret
  	.simple_format_byte:
        sub byte[ecx], 100
        add byte[ecx-2], 1
        jmp .finish_simple_format_byte
format:;转十进制
    push edx
	mov ebx, eax
	.toEnd:
		cmp byte[eax], 0
		je .find_position
		inc eax
		jmp .toEnd
	.find_position:
		dec eax
		cmp eax,ebx
		je .formatFinish
	.formatLoop_1:
		cmp byte[eax], '9'
		jna .find_position
		mov edx, eax
		dec edx
		sub byte[eax], 10
		add byte[edx], 1
		jmp .formatLoop_1
	.formatFinish:
		mov ecx,0
		cmp byte[eax],'9'
		ja .extraPrint
		pop edx
		ret
	.extraPrint:
		inc ecx
		sub byte[eax],10
		cmp byte[eax],'9'
		ja .extraPrint
		push eax
		mov eax, ecx
		call printnumber
		pop eax
		jmp .formatFinish
print:;打印数字
	cmp byte[eax+1], 0
	je print_eax
	cmp byte[eax+1], '0'
	jb print_eax 
	cmp byte[eax+1], '9'
	ja print_eax 
	cmp byte[eax], '0'
	ja print_eax
	inc eax
	jmp print
newline:;新一行
    pushad
    mov edx, 1
    mov ecx, line
    mov ebx, 1
    mov eax, 4
    int 80h
    popad
    ret
clear:;完成一次计算后进行清空
    pushad
    mov eax, input_number
    call clearone
    mov eax, first_number
    call clearone  
    mov eax, second_number
    call clearone
    mov eax, sum
    call clearone
    mov eax, product
    call clearone
    mov eax, operator
    call clearoperator
    popad
    ret