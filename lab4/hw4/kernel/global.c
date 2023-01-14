
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            global.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#define GLOBAL_VARIABLES_HERE

#include "type.h"
#include "const.h"
#include "protect.h"
#include "tty.h"
#include "console.h"
#include "proc.h"
#include "global.h"
#include "proto.h"


PUBLIC	PROCESS	proc_table[NR_TASKS + NR_PROCS];

PUBLIC	TASK	task_table[NR_TASKS] = {
	{task_tty, STACK_SIZE_TTY, "tty"}};

PUBLIC  TASK    user_proc_table[NR_PROCS] = {
					{ReporterA, STACK_SIZE_TESTA, "ReporterA"},
					{ReaderB, STACK_SIZE_TESTB, "ReaderB"},
					{ReaderC, STACK_SIZE_TESTC, "ReaderC"},
                    {ReaderD, STACK_SIZE_TESTD, "ReaderD"},
                    {WriterE, STACK_SIZE_TESTE, "WriterE"},
                    {WriterF, STACK_SIZE_TESTF, "WriterF"}};

PUBLIC	char		task_stack[STACK_SIZE_TOTAL];

PUBLIC	TTY		tty_table[NR_CONSOLES];
PUBLIC	CONSOLE		console_table[NR_CONSOLES];

PUBLIC	irq_handler	irq_table[NR_IRQ];

PUBLIC	system_call	sys_call_table[NR_SYS_CALL] = {
        sys_get_ticks,
        sys_write_str,
        sys_sleep,
        p_process,
        v_process
};

PUBLIC  SEMAPHORE rw_mutex = {1, 0, 0};
PUBLIC  SEMAPHORE w_mutex = {1, 0, 0};
PUBLIC  SEMAPHORE r_mutex = {1, 0, 0};
PUBLIC  SEMAPHORE queue = {1, 0, 0};
PUBLIC  SEMAPHORE n_r_mutex = {MAX_READERS, 0, 0};