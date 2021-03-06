#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "nemu.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

void cpu_exec(uint32_t);

void GetFunctionAddr(swaddr_t EIP,char* name);
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
	WP *head = getHead();
	while(head!=NULL){
		printf("watchpoint %d : %s\n", head->NO, head->expr);
		head = head->next;
	}
}

//read address
uint32_t read_address(char *args){
	uint32_t address;
	address = 0;
	while(('0' <= args[0] && args[0] <= '9') || ('a' <= args[0] && args[0] <= 'f') || ('A' <= args[0] && args[0] <= 'F')){
				if('0' <= args[0] && args[0] <= '9') address = (address<<4)+((args[0]-'0'));
				if('a' <= args[0] && args[0] <= 'f') address = (address<<4)+((args[0]-'a')+9);
				if('A' <= args[0] && args[0] <= 'F') address = (address<<4)+((args[0]-'A')+9);
				++args;
			}

	return address;
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

// static int cmd_x(char* args){
// 	if(args == NULL) return 0;
// 	uint32_t num = 0, addr;
// 	while(args[0] == ' ')++args;	//trim
// 	while('0' <= args[0] && args[0] <= '9') num = (num << 3) + (num << 1) + (args[0] & 15), ++args;
// 	//get num
// 	while(args[0] == ' ')++args; //trim
// 	//get address
// 	if(args[0] == '0' && args[1] == 'x'){
// 		args = args + 2;
// 		addr = read_address(args);

// 		while(num) {
// 			printf("address 0x%x:", addr);
// 			int i;
// 			for(i = 0;i < 4; i++)printf(" 0x%x", swaddr_read(addr + i, 1));
// 			printf("\n");
// 			addr += 4;
// 			--num;
// 		}
// 		return 0;
// 	}else{
// 		printf("\033[1;31mInvalid expression\n\033[0m");
// 		return 0;
// 	}

// }
 static int cmd_x(char *args){
	if(args == NULL) return 0;
	uint32_t num = 0, addr;
	bool suc;
	while(args[0] == ' ')++args;	//trim
	while('0' <= args[0] && args[0] <= '9') num = (num << 3) + (num << 1) + (args[0] & 15), ++args;
	//get number
	addr = expr(args, &suc);
	if(!suc) {
		printf("\033[1;31mInvalid expression\n\033[0m");
		return 0;
	}
	while(num) {
		printf("address 0x%x:", addr);
		int i;
		for(i = 0;i < 4; i++)printf(" 0x%x", swaddr_read(addr + i, 1));
		printf("\n");
		addr += 4;
		--num;
	}
	return 0;

	}

static int cmd_p(char *args) {
	if(args == NULL) return 0;
	bool suc;
	uint32_t ans = expr(args, &suc);	//fix bugs
	if(!suc) {
		printf("\033[1;31mInvalid expression\n\033[0m");
		return 0;
	}
	//tokens;
	printf("Expression %s : 0x%x\n", args, ans);
	return 0;
}

static int cmd_w(char *args) {
	if(args == NULL) return 0;
	int id = insertExpr(args);
	if(id == -1) {
		printf("\033[1;31mInvalid expression\n\033[0m");
		return 0;
	}
	printf("Add watchpoint %d\n", id);
	return 0;
}

static int cmd_b(char *args){
	if(args == NULL){
		printf("Please enter an address\n");
		return 0;
	}else
	{
		cmd_w(args);
		return 1;
	}
	
}

static int cmd_d(char *args) {
	if(args == NULL) return 0;
	int id;
	sscanf(args, "%d", &id);
	int ans = removeNode(id);//remove a node
	if(ans == 0) {
		printf("\033[1;31mWatchpoint %d doesn't exist\n\033[0m", id);
	} else {
		printf("Delete watchpoint %d successfully\n", id);
	}
	return 0;
}


typedef struct {
	swaddr_t prev_ebp;
	swaddr_t ret_addr;
	uint32_t args[4];
}PartOfStackFrame ;
static int cmd_bt(char* args){
	if (args != NULL){
		printf("Wrong Command!");
		return 0;
	}
	PartOfStackFrame EBP;
	char name[32];
	int cnt = 0;
	EBP.ret_addr = cpu.eip;
	swaddr_t addr = cpu.ebp;
	// printf("%d\n",addr);
	int i;
	while (addr){
		GetFunctionAddr(EBP.ret_addr,name);
		if (name[0] == '\0') break;
		printf("#%d\t0x%08x\t",cnt++,EBP.ret_addr);
		printf("%s",name);
		EBP.prev_ebp = swaddr_read(addr,4);
		EBP.ret_addr = swaddr_read(addr + 4, 4);
		printf("(");
		for (i = 0;i < 4;i ++){
			EBP.args[i] = swaddr_read(addr + 8 + i * 4, 4);
			printf("0x%x",EBP.args[i]);
			if (i == 3) printf(")\n");else printf(", ");
		}
		addr = EBP.prev_ebp;
	}
	return 0;
}



static struct {
	char *name;
	char *description;
	int (*handler) (char *);
} cmd_table [] = {
	{ "help", "Display informations about all supported commands", cmd_help },
	{ "c", "Continue the execution of the program", cmd_c },
	{ "q", "Exit NEMU", cmd_q },
	{ "si", "Step into implementation of N instructions after the suspension of execution.When N is notgiven,the default is 1.", cmd_si},
	{ "info", "r for print register state \n w for print watchpoint information", cmd_info},
	{ "b", "Breakpoint + *ADDR.", cmd_b},
	{ "p", "Expression evaluation", cmd_p},
	{ "x", "Calculate the value of the expression and regard the result as the starting memory address.", cmd_x},
	{ "w", "Stop the execution of the program if the result of the expression has changed.", cmd_w},
	{ "d", "Delete the Nth watchpoint", cmd_d},
	{ "bt", "Print stack frame chain", cmd_bt},

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
