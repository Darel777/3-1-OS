
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
			      console.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
						    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/*
	回车键: 把光标移到第一列
	换行键: 把光标前进到下一行
*/


#include "type.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "keyboard.h"
#include "proto.h"

PRIVATE void set_cursor(unsigned int position);
PRIVATE void set_video_start_addr(u32 addr);
PRIVATE void flush(CONSOLE* p_con);
PUBLIC void push_pos_stack(CONSOLE* p_con,int pos);
PUBLIC int pop_pos_stack(CONSOLE* p_con);
PUBLIC void push_char_stack(CONSOLE* p_con,char ch);
PUBLIC void redo(CONSOLE* p_con);
PUBLIC void search2result(CONSOLE* p_con);
PUBLIC void result2normal(CONSOLE* p_con);

PUBLIC void push_pos_stack(CONSOLE* p_con,int pos){//位置入栈
	p_con->pos_stack.pos[p_con->pos_stack.offset] = pos;
	p_con->pos_stack.offset+=1;
}
PUBLIC int pop_pos_stack(CONSOLE* p_con){//位置出栈
	if(p_con->pos_stack.offset){
		p_con->pos_stack.offset-=1;
		return p_con->pos_stack.pos[p_con->pos_stack.offset];
	}else{
		return 0;
	}
}
PUBLIC void push_char_stack(CONSOLE* p_con,char ch){//char入栈-》为什么不存颜色？因为打字都是白色，并且颜色显式上只有封装好的红白变化
	p_con->char_stack.ch[p_con->char_stack.offset]=ch;
	p_con->char_stack.offset+=1;
}
PUBLIC void redo(CONSOLE *p_con){
	int start;
	if(mode==0){
		start = 0;
	}else if(mode==1){
		start = p_con->char_stack.search;
	}else{
		return;
	}//mode=2 不进行redo
	p_con->char_stack.offset-=2; // ctrl + 'z'
	if(p_con->char_stack.offset<=start){
		p_con->char_stack.offset=start;
		return;		
	} 
	for(int i=start;i<p_con->char_stack.offset;i++){
		out_char(p_con,p_con->char_stack.ch[i]);
	}
}

PUBLIC void search2result(CONSOLE *p_con){
	int start,end;
	for(int i = 0; i < p_con->search_start_pos*2;i+=2){ // 遍历原始白色输入
		start = end = i; 
		int found = 1; 
		for(int j = p_con->search_start_pos*2;j<p_con->cursor*2;j+=2){
			if(*((u8*)(V_MEM_BASE+end))==' '){ 
				if(*((u8*)(V_MEM_BASE+j))!=' '){ 
					found = 0 ;
					break;
				}
				if(*((u8*)(V_MEM_BASE+end+1))==BLUE){//虽然是TAB 也一个一个空格进行对照 因为j是不变的
					if(*((u8*)(V_MEM_BASE+j+1))==BLUE){
						end+=2;
					}else{
						found = 0;
						break;
					}
				}else{ 
					end+=2;
				}
			}
			else if(*((u8*)(V_MEM_BASE+end))==*((u8*)(V_MEM_BASE+j))){
				end+=2;
			}else{
				found = 0;
				break;
			}
		}
		if(found == 1){
			for(int k = start;k<end;k+=2){
				*(u8*)(V_MEM_BASE + k + 1) = RED;
			}
		}
	}
}

PUBLIC void result2normal(CONSOLE* p_con){
	u8* p_vmem = (u8*)(V_MEM_BASE + p_con->cursor * 2);//vmem的打字的尽头
	for(int i=0;i<p_con->cursor-p_con->search_start_pos;++i){ 
		*(p_vmem-2-2*i) = ' ';
		*(p_vmem-1-2*i) = WHITE;
	}//从搜索输入向前
	for(int i=0;i<p_con->search_start_pos*2;i+=2){ 
		*(u8*)(V_MEM_BASE + i + 1) = WHITE;
	}//从标准输入向后
	p_con->cursor = p_con->search_start_pos;
	p_con->pos_stack.offset = p_con->pos_stack.search;
	p_con->char_stack.offset = p_con->char_stack.search;
	flush(p_con);
}
PUBLIC void out_char(CONSOLE* p_con, char ch)
{
	u8* p_vmem = (u8*)(V_MEM_BASE + p_con->cursor * 2);
	switch(ch) {
	case '\t': //TAB 添加的是入栈
		if(p_con->cursor < p_con->original_addr + p_con->v_mem_limit - 4){
			for(int i=0;i<4;i++){ 
				*p_vmem++ = ' ';
				*p_vmem++ = BLUE;
			}
			push_pos_stack(p_con,p_con->cursor);
			p_con->cursor += 4; 
		}
		break;	
	case '\n'://回车 添加的是入栈
		if (p_con->cursor < p_con->original_addr + p_con->v_mem_limit - SCREEN_WIDTH) {
			push_pos_stack(p_con,p_con->cursor);
			p_con->cursor = p_con->original_addr + SCREEN_WIDTH * 
				((p_con->cursor - p_con->original_addr) /
				 SCREEN_WIDTH + 1);
		}
		break;
	case '\b'://退格 添加的是出栈
		if (p_con->cursor > p_con->original_addr) {
			int temp = p_con->cursor; 
			p_con->cursor = pop_pos_stack(p_con);
			int i;
			for(i=0;i<temp-p_con->cursor;i++){ 
				*(p_vmem-2-2*i) = ' ';
				*(p_vmem-1-2*i) = WHITE;
			}
		}
		break;
	case 'z':
	case 'Z':
		if(control&&(mode==0||mode==1)){ 
			int start;
			if(mode==0){
				start = 0;	
				p_con->pos_stack.offset = 0;
			}else if(mode==1){
				start = p_con->search_start_pos*2;
				p_con->pos_stack.offset = p_con->pos_stack.search;
			}
			for (int i = 0 ; i < SCREEN_SIZE; i++){
				disp_str(" ");
			}
			disp_pos = start;
			p_con->cursor = disp_pos / 2;
			flush(p_con);
			redo(p_con);
			return; 
		}
	default:
		if (p_con->cursor < p_con->original_addr + p_con->v_mem_limit - 1) {
			*p_vmem++ = ch;
			if(mode==0){
				*p_vmem++ = WHITE;
			}else{
				*p_vmem++ = RED;
			}
			push_pos_stack(p_con,p_con->cursor);
			p_con->cursor++;
		}
		break;
	}

	while (p_con->cursor >= p_con->current_start_addr + SCREEN_SIZE) {
		scroll_screen(p_con, SCR_DN);
	}

	flush(p_con);
}


/*======================================================================*
                           flush
*======================================================================*/
PRIVATE void flush(CONSOLE* p_con)
{
        set_cursor(p_con->cursor);
        set_video_start_addr(p_con->current_start_addr);
}

/*======================================================================*
			    set_cursor
 *======================================================================*/
PRIVATE void set_cursor(unsigned int position)
{
	disable_int();
	out_byte(CRTC_ADDR_REG, CURSOR_H);
	out_byte(CRTC_DATA_REG, (position >> 8) & 0xFF);
	out_byte(CRTC_ADDR_REG, CURSOR_L);
	out_byte(CRTC_DATA_REG, position & 0xFF);
	enable_int();
}

/*======================================================================*
			  set_video_start_addr
 *======================================================================*/
PRIVATE void set_video_start_addr(u32 addr)
{
	disable_int();
	out_byte(CRTC_ADDR_REG, START_ADDR_H);
	out_byte(CRTC_DATA_REG, (addr >> 8) & 0xFF);
	out_byte(CRTC_ADDR_REG, START_ADDR_L);
	out_byte(CRTC_DATA_REG, addr & 0xFF);
	enable_int();
}



/*======================================================================*
			   select_console
 *======================================================================*/
PUBLIC void select_console(int nr_console)	/* 0 ~ (NR_CONSOLES - 1) */
{
	if ((nr_console < 0) || (nr_console >= NR_CONSOLES)) {
		return;
	}

	nr_current_console = nr_console;

	set_cursor(console_table[nr_console].cursor);
	set_video_start_addr(console_table[nr_console].current_start_addr);
}

/*======================================================================*
			   scroll_screen
 *----------------------------------------------------------------------*
 滚屏.
 *----------------------------------------------------------------------*
 direction:
	SCR_UP	: 向上滚屏
	SCR_DN	: 向下滚屏
	其它	: 不做处理
 *======================================================================*/
PUBLIC void scroll_screen(CONSOLE* p_con, int direction)
{
	if (direction == SCR_UP) {
		if (p_con->current_start_addr > p_con->original_addr) {
			p_con->current_start_addr -= SCREEN_WIDTH;
		}
	}
	else if (direction == SCR_DN) {
		if (p_con->current_start_addr + SCREEN_SIZE <
		    p_con->original_addr + p_con->v_mem_limit) {
			p_con->current_start_addr += SCREEN_WIDTH;
		}
	}
	else{
	}

	set_video_start_addr(p_con->current_start_addr);
	set_cursor(p_con->cursor);
}

PUBLIC int is_current_console(CONSOLE* p_con)
{
	return (p_con == &console_table[nr_current_console]);
}

PUBLIC void init_screen(TTY* p_tty)
{
	int nr_tty = p_tty - tty_table;
	p_tty->p_console = console_table + nr_tty;

	int v_mem_size = V_MEM_SIZE >> 1;	/* 显存总大小 (in WORD) */

	int con_v_mem_size                   = v_mem_size / NR_CONSOLES;
	p_tty->p_console->original_addr      = nr_tty * con_v_mem_size;
	p_tty->p_console->v_mem_limit        = con_v_mem_size;
	p_tty->p_console->current_start_addr = p_tty->p_console->original_addr;
	// 初始化pos_stack的ptr指针
	p_tty->p_console->pos_stack.offset = 0;
	// 初始化out_char_stack
	p_tty->p_console->char_stack.offset = 0;


	/* 默认光标位置在最开始处 */
	p_tty->p_console->cursor = p_tty->p_console->original_addr;

	if (nr_tty == 0) {
		/* 第一个控制台沿用原来的光标位置 */
		p_tty->p_console->cursor = disp_pos / 2;
		disp_pos = 0;
	}
	else {
		out_char(p_tty->p_console, nr_tty + '0');
		out_char(p_tty->p_console, '#');
	}

	set_cursor(p_tty->p_console->cursor);
}
