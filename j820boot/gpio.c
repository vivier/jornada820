#include "tester1.h"
#include "setup.h"

/* THIS FILE IS NOT WRITEEN FOR SA1100 AND SHOULD BE ADAPTED */

#define GPIO	0x40E00000

int GPIOalt[81],GPIOio[81],GPIOst[81],mediaq;

void SetGPIOio(int i,int io) // set GPIO as io == 1 output  io == 0 input
{
	UINT32 *gpio=(UINT32*)VirtualAlloc((void*)0x0,0x100, MEM_RESERVE,PAGE_READWRITE);
	VirtualCopy((void*)gpio,(void *) (GPIO/256),	0x100, PAGE_READWRITE|PAGE_NOCACHE|PAGE_PHYSICAL);
	if(io)
		gpio[0xC/4 + (i/32)] = gpio[0xC/4 + (i/32)] | (1<<(i%32));
	else
		gpio[0xC/4 + (i/32)] = gpio[0xC/4 + (i/32)] & (~(1<<(i%32)));
	VirtualFree(gpio,0,MEM_RELEASE);
}

void SetGPIOalt(int i,int io) // set GPIO as io == 1 output  io == 0 input
{
	UINT32 *gpio=(UINT32*)VirtualAlloc((void*)0x0,0x100, MEM_RESERVE,PAGE_READWRITE);
	VirtualCopy((void*)gpio,(void *) (GPIO/256),	0x100, PAGE_READWRITE|PAGE_NOCACHE|PAGE_PHYSICAL);
	UINT32 w = gpio[0x54/4 + (i/16)] & (~(3<<((i%16)*2)));
	gpio[0x54/4 + (i/16)] = w | (io<<((i%16)*2));
	VirtualFree(gpio,0,MEM_RELEASE);
}

void writeGPIOConfig(FILE*fd, UINT32 *gpio)
{
	for(int i=0;i<81;i++)
	{
		UINT32 adr=i/16;
		fprintf(fd,"GPIO #%d=%c, %d\n",i,(gpio[0xC/4+(adr/2)]>>(i%32))&0x1?'O':'I',(gpio[0x54/4+adr]>>((i%16)*2))&0x3);
		GPIOalt[i]=(gpio[0x54/4+adr]>>((i%16)*2))&0x3;
		GPIOio[i]=(gpio[0xC/4+(adr/2)]>>(i%32))&0x1;

	}
}

inline int readGPIOstate(UINT32 *gpio,int i)
{
	return (gpio[(i/32)]>>(i%32))&0x1;
}

inline int readGPIOalt(UINT32 *gpio,int i)
{
	return (gpio[0x54/4+i/16]>>((i%16)*2))&0x3;
}

void Gpio()
{
	SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_BELOW_NORMAL);
//	SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_TIME_CRITICAL);
	FILE *fd=fopen("\\loggerG.txt","w");
	SYSTEMTIME time;
	
	fprintf(fd,"MCMEM0=0x%x\n",ReadPhysical(0x48000028));
	fprintf(fd,"MCMEM1=x%x\n",ReadPhysical(0x4800002C));
	fprintf(fd,"MCATT0=0x%x\n",ReadPhysical(0x48000030));
	fprintf(fd,"MCATT1=0x%x\n",ReadPhysical(0x48000034));
	fprintf(fd,"MCIO0=0x%x\n",ReadPhysical(0x48000038));
	fprintf(fd,"MCIO1=0x%x\n",ReadPhysical(0x4800003C));
	
	for(int q=0;q<16;q++)
		fprintf(fd,"DMA #%d=0x%x\tDCMD=0x%x\tDSADR=0x%x\tDTADR=0x%x\n",q,ReadPhysical(0x40000000+q*4),ReadPhysical(0x40000200+q*0x10+0xC),ReadPhysical(0x40000200+q*0x10+0x4),ReadPhysical(0x40000200+q*0x10+0x8));
	GetSystemTime(&time);
	int sec=time.wSecond,quit=0;
	sec+=4;
	if(sec>60) sec-=60;
	UINT32 *gpio=(UINT32*)VirtualAlloc((void*)0x0,0x100, MEM_RESERVE,PAGE_READWRITE);
	VirtualCopy((void*)gpio,(void *) (GPIO/256),	0x100, PAGE_READWRITE|PAGE_NOCACHE|PAGE_PHYSICAL);
	UINT32 *gpio1=(UINT32*)VirtualAlloc((void*)0x0,0x1000, MEM_RESERVE,PAGE_READWRITE);
	VirtualCopy((void*)gpio1,(void *) (GPIO_MEDIAQ/256),	0x1000, PAGE_READWRITE|PAGE_NOCACHE|PAGE_PHYSICAL);
	writeGPIOConfig(fd,gpio);

 	for(int i=0;i<81;i++)
	{
		GPIOst[i]=-1;
	}
	int state,alt;
	while(sec>time.wSecond)
	{
		GetSystemTime(&time);
		if(gpio1[MQ_OFF/4]!=(UINT32)mediaq)
		{
			mediaq=gpio1[MQ_OFF/4];
			fprintf(fd,"MQ_GPIO=0x%x\n",mediaq);
		}
		for(i=0;i<81;i++)
		{
/*		if(GPIOio[i]) continue;*/
		if(GPIOalt[i]) continue;
//			if(i==16) continue;
//			if(i==28) continue;
//			if(i==29) continue;
//			if(i==30) continue;
//			if(i==31) continue;
			state=readGPIOstate(gpio,i);
			alt=readGPIOalt(gpio,i);
			if(GPIOst[i]==state&&GPIOalt[i]==alt) continue;
			GPIOst[i]=state;
			if(GPIOalt[i]==alt)
				fprintf(fd,"#%d: %d\n",i,state);
			else
			{
				GPIOalt[i]=alt;
				fprintf(fd,"#%d: %d\tf=%d\n",i,state,alt);
			}

		}
	}
	fclose(fd);
}
