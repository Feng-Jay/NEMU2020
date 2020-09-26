#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "nemu.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

void cpu_exec(uint32_t);

/* We use the `readline' library to provide more flexibility to read from stdin. */
char* rl_gets() {
	static char *line_read = NULL;

	if (line_read) {
		free(line_read);
		line_read = NULL;
	}

	line_read = readline("(nemu) ");

	if (line_read && *line_read) {
		add_history(line_read);
	}

	return line_read;
}

void printRegisters(){
	printf("eax: 0x%-10x  %-10d\n", cpu.eax, cpu.eax);
	printf("edx: 0x%-10x  %-10d\n", cpu.edx, cpu.edx);
	printf("ecx: 0x%-10x  %-10d\n", cpu.ecx, cpu.ecx);
	printf("ebx: 0x%-10x  %-10d\n", cpu.ebx, cpu.ebx);
	printf("ebp: 0x%-10x  %-10d\n", cpu.ebp, cpu.ebp);
	printf("esi: 0x%-10x  %-10d\n", cpu.esi, cpu.esi);
	printf("esp: 0x%-10x  %-10d\n", cpu.esp, cpu.esp);
	printf("eip: 0x%-10x  %-10d\n", cpu.eip, cpu.eip);

}

void display_wp(){
	printf("hello");
}

static int cmd_c(char *args) {
	cpu_exec(-1);
	return 0;
}

static int cmd_q(char *args) {
	return -1;
}

static int cmd_help(char *args);

static int cmd_si(char *args){
	//initliza the step
	int step = 0;
	if(args == NULL)step=1;
	else
	{
		sscanf(args,"%d",&step);

	}
	cpu_exec(step);
	return 0;
	
}

 static int cmd_info(char *args){
	if(args == NULL){
		printf("Please input the info r or info w\n");
	
	}else if(args[0] == 'r'){
		printRegisters();
	}else if(args[0] == 'w'){
		display_wp();
	}else
	{
		printf("The info command need a parameter 'r' or 'w'\n");
	}
	return 0;
	
	
	 
 }

 static int cmd_x(char *args){
	
	if(args == NULL){
		printf("too few parameter! \n");
		return 1;
	}else
	{
		char *arg = strtok(args, " ");
		if(arg == NULL){
			printf("too few parameter! \n");
			return 1;
		}

		int n = atoi(arg);
		char *EXPR = strtok(NULL, " ");
		if(EXPR == NULL){
			printf("too few parameter! \n");
			return 1;
		}

		if(strtok(NULL, " ")!=NULL){
			printf("too many parameter! \n");
			return 1;
		}

		bool success = true;

		if(!success){
			printf("ERROR!\n");
			return 1;
		}

		char *str;
		swaddr_t addr = strtol(EXPR,&str,16);

		int i;
		for(i=0;i<n;i++){
			uint32_t data = swaddr_read(addr+i*4,4);
			printf("0x%08x ", addr+i*4);
			int j;
			for(j=0; j<4; j++){
				printf("0x%02x ",data&0xff);
				data = data>>8;
			}
			printf("\n");
		}

		return 0;


	}
	
}

static int cmd_p(char *args){
	uint32_t num;
	bool success;
	num = expr(args,&success);
	if(success){
		printf("0x%x:\t%d\n",num,num);
	}else
	{
		assert(0);
	}

	return 0;
	
}

// static int cmd_w(char *args);

// static int cmd_b(char *args);

// static int cmd_d(char *args);

// static int cmd_bt(char *args);

// static int cmd_cache(char *args);

static struct {
	char *name;
	char *description;
	int (*handler) (char *);
} cmd_table [] = {
	{ "help", "Display informations about all supported commands", cmd_help },
	{ "c", "Continue the execution of the program", cmd_c },
	{ "q", "Exit NEMU", cmd_q },

	/* TODO: Add more commands */
	/*Now start to write dbq!!! 9.26*/

	{ "si", "Step into implementation of N instructions after the suspension of execution.When N is notgiven,the default is 1.", cmd_si},
	{ "info", "r for print register state \n w for print watchpoint information", cmd_info},
	// { "b", "Breakpoint + *ADDR.", cmd_b},
	{ "p", "Expression evaluation", cmd_p},
	{ "x", "Calculate the value of the expression and regard the result as the starting memory address.", cmd_x},
	// { "w", "Stop the execution of the program if the result of the expression has changed.", cmd_w},
	// { "d", "Delete the Nth watchpoint", cmd_d},
	// { "bt", "Print stack frame chain", cmd_bt},
	// { "cache", "Print cache block infomation", cmd_cache}

};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

static int cmd_help(char *args) {
	/* extract the first argument */
	char *arg = strtok(NULL, " ");
	int i;

	if(arg == NULL) {
		/* no argument given */
		for(i = 0; i < NR_CMD; i ++) {
			printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
		}
	}
	else {
		for(i = 0; i < NR_CMD; i ++) {
			if(strcmp(arg, cmd_table[i].name) == 0) {
				printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
				return 0;
			}
		}
		printf("Unknown command '%s'\n", arg);
	}
	return 0;
}

void ui_mainloop() {
	while(1) {
		char *str = rl_gets();
		char *str_end = str + strlen(str);

		/* extract the first token as the command */
		char *cmd = strtok(str, " ");
		if(cmd == NULL) { continue; }

		/* treat the remaining string as the arguments,
		 * which may need further parsing
		 */
		char *args = cmd + strlen(cmd) + 1;
		if(args >= str_end) {
			args = NULL;
		}

#ifdef HAS_DEVICE
		extern void sdl_clear_event_queue(void);
		sdl_clear_event_queue();
#endif

		int i;
		for(i = 0; i < NR_CMD; i ++) {
			if(strcmp(cmd, cmd_table[i].name) == 0) {
				if(cmd_table[i].handler(args) < 0) { return; }
				break;
			}
		}

		if(i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
	}
}
