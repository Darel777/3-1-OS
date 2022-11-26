
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
			      console.h
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
						    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#ifndef _ORANGES_CONSOLE_H_
#define _ORANGES_CONSOLE_H_


#define SCR_UP	1	
#define SCR_DN	-1	

#define SCREEN_SIZE		(80 * 25)
#define SCREEN_WIDTH		80



typedef struct out //存储文字记录栈
{
	int offset;
	char ch[SCREEN_SIZE];
	int search;
}OUTCHAR;
typedef struct cursor //光标位置栈
{
	int offset;
	int pos[SCREEN_SIZE]; 
	int search;
}POS;
typedef struct s_console
{
	unsigned int	current_start_addr;	/* 当前显示到了什么位置	  */
	unsigned int	original_addr;		/* 当前控制台对应显存位置 */
	unsigned int	v_mem_limit;		/* 当前控制台占的显存大小 */
	unsigned int	cursor;				/* 当前光标位置 */
	unsigned int 	search_start_pos;	
	POS pos_stack;						
	OUTCHAR char_stack;		
}CONSOLE;

#endif /* _ORANGES_CONSOLE_H_ */
