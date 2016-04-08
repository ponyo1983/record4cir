
#include "state.h"

static char state = 0x17;

char get_sys_state() {
	return state;
}

void set_sys_state(char bit, char state) {

	int tmp;
	if(state==0)
	{
		tmp=~(1<<bit);
		state=state&tmp;
	}
	else if(state==1)
	{
		state=state|(1<<bit);
	}
}
