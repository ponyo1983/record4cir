
#include "state.h"

static char bd_state = 0x17;

char get_sys_state() {
	return bd_state;
}

void set_sys_state(char bit, char state) {

	int tmp;
	if(state==0)
	{
		tmp=~(1<<bit);
		bd_state=bd_state&tmp;
	}
	else if(state==1)
	{
		bd_state=bd_state|(1<<bit);
	}
}
