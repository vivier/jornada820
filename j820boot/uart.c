
#include "tester1.h"
#include "setup.h"

#define FUART 0x40100000

void UART_puts(char *s)
{
	UINT32 *base=(UINT32*)VirtualAlloc((void*)0x0,sizeof(void*)*0xffff, MEM_RESERVE,PAGE_READWRITE);
	int ret=VirtualCopy(base,(void *) ((FUART)/256),sizeof(void*)*0xffff	, PAGE_READWRITE|PAGE_NOCACHE|PAGE_PHYSICAL);
	int a=0;
	while(s[a])
	{
		while(!(base[0x14/4]&1<<5)) {}
		base[0]=(char)(s[a]);
		a++;
	}
}

void UART_setup()
{
	UINT32 *base=(UINT32*)VirtualAlloc((void*)0x0,sizeof(void*)*0xffff, MEM_RESERVE,PAGE_READWRITE);
	int ret=VirtualCopy(base,(void *) ((FUART)/256),sizeof(void*)*0xffff	, PAGE_READWRITE|PAGE_NOCACHE|PAGE_PHYSICAL);

// set DLAB
	base[0x0C/4]=128+2+1;
// set divisor
	base[0]=8; // 115200 bps
	base[0x04/4]=0;
// unset DLAB
	base[0x0C/4]=2+1;
// UART enable & no FIFO
	base[0x04/4]=64;
	base[0x08/4]=0;

	char test[]="LinExec: UART Initialized.\n\r";
	int a=0;
	while(test[a])
	{
		while(!(base[0x14/4]&1<<5)) {}
		base[0]=(char)(test[a]);
		a++;
	}
}
