#include "tester1.h"
#include "setup.h"


void setup_linux_params(long bootimg_dest, UINT32 initrd,UINT32 initrdl, long dram_size, const char *cmdline, char*base)
{
	int rootdev = 0x00ff;
	struct tag *tag;
	int newcmdlinelen = 0;
	char *newcmdline = NULL;


	tag = (struct tag *)(base+0x100);

	tag->hdr.tag = ATAG_CORE;
	tag->hdr.size = tag_size(tag_core);
	tag->u.core.flags =0;
	tag->u.core.pagesize = 0x00001000;
	tag->u.core.rootdev = rootdev;
	tag = tag_next(tag);

	// now the cmdline tag
	tag->hdr.tag = ATAG_CMDLINE;
	// must be at least +3!! 1 for the null and 2 for the ???
	tag->hdr.size = (strlen(cmdline) + 3 + sizeof(struct tag_header)) >> 2;
	//tag->hdr.size = (strlen(cmdline) + 10 + sizeof(struct tag_header)) >> 2;
	strcpy(tag->u.cmdline.cmdline,cmdline);
	tag = tag_next(tag);


	// now the mem32 tag
	tag->hdr.tag = ATAG_MEM;
	tag->hdr.size = tag_size(tag_mem32);
	tag->u.mem.size = dram_size;
	tag->u.mem.start = MEM_START;
	tag = tag_next(tag);
       

	/* and now the initrd tag */
	if (initrdl) {
		tag->hdr.tag = INITRD_TAG;
		tag->hdr.size = tag_size(tag_initrd);
		tag->u.initrd.start = initrd;
		tag->u.initrd.size = initrdl;
		tag = tag_next(tag);
	}
   
	tag->hdr.tag = ATAG_VIDEOTEXT;
	tag->hdr.size = tag_size(tag_videotext);
	tag->u.videotext.video_lines = 40;
	tag->u.videotext.video_cols = 30;
	tag = tag_next(tag);

	// now the NULL tag
	tag->hdr.tag = ATAG_NONE;
	tag->hdr.size = 0;
}





/* loading process:
function do_it is loaded onto address KERNELCOPY along with parameters(offset=0x100) and
kernel image(offset=0x8000). Afterwards DRAMloader is called; it disables MMU and
jumps onto KERNELCOPY. Function do_it then copies kernel image to its proper address(0xA0008000) 
and calls it.
Initrd is loaded onto address INITRD and the address is passed to kernel via ATAG
*/


// This resets some devices
void ResetDevices()
{
#ifndef STRONGARM
	WritePhysical(0x4050000C,0); // Reset AC97
	WritePhysical(0x48000014,0); // Reset PCMCIA
	for(int i=0;i<0x3C;i+=4)
		WritePhysical(0x40000000,8); // Set DMAs to Stop state
	WritePhysical(0x400000F0,0); // DMA do not gen interrupt
	SetGPIOio(28,0);			// AC97
	SetGPIOio(29,0);			// AC97/I2S
	SetGPIOio(30,0);			// I2S/AC97
	SetGPIOio(31,0);			// I2S/AC97
	SetGPIOio(32,0);			// AC97/I2S
	SetGPIOalt(28,0);
	SetGPIOalt(29,0);
	SetGPIOalt(30,0);
	SetGPIOalt(31,0);
	SetGPIOalt(32,0);
#endif
}




void mymemcpy(char* a, char* b, int size);

void boot_linux(char *filename,char* initrd,char *param)
{
	FILE *fd=fopen(filename,"rb");
	int ret;

	FILE* fd1;

	long initrdl;
	long len;

	if(!fd)
	{
		FILE *logfd=fopen("\\bootlog.txt","w");
		fprintf(logfd, "Booting: ***FAILED TO OPEN %s***\n",filename);
		fclose(logfd);
		return;
	}

	fseek(fd,0,SEEK_END);
	len=ftell(fd);
	fseek(fd,0,SEEK_SET);

	fd1=fopen(initrd,"rb");
	initrdl=0;
	if(fd1) 
	{
		fseek(fd1,0,SEEK_END);
		initrdl=ftell(fd1);
		fseek(fd1,0,SEEK_SET);
	}
	FILE *logfd=fopen("\\bootlog.txt","w");
	fprintf(logfd, "Booting: Images.");
	fclose(logfd);
	
	logfd=fopen("\\bootlog.txt","w");
	fprintf(logfd, "Booting: entering supervisor mode.");
	fclose(logfd);

	/* now becoming supervisor. */
	SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_TIME_CRITICAL);
	CeSetThreadQuantum(GetCurrentThread(),0);
	SetKMode(1);
  	SetProcPermissions(0xffffffff);
	/* <ibot> rooooooooot has landed! */

	logfd=fopen("\\bootlog.txt","w");
	fprintf(logfd, "Booting: supervisor mode.");
	fclose(logfd);

	void *mmu=(void*)read_mmu();
	UINT32 *data=NULL,*lcd=NULL,*intr=NULL,*_mmu=NULL;
	char *watch=NULL,*krnl=NULL;


	IntOff();


	char *kernel_copy2=(char*)VirtualAlloc((void*)0x0,0x8000+len, MEM_RESERVE|MEM_TOP_DOWN,PAGE_READWRITE);
	ret=VirtualCopy((void*)kernel_copy2,(void *) (KERNELCOPY/256),	0x8000+len, PAGE_READWRITE|PAGE_NOCACHE|PAGE_PHYSICAL);

	char *initrd_copy2;


	if(fd1)
	{
		initrd_copy2=(char*)VirtualAlloc((void*)0x0,initrdl, MEM_RESERVE|MEM_TOP_DOWN,PAGE_READWRITE);
		ret=VirtualCopy((void*)initrd_copy2,(void *) (INITRD/256),	initrdl, PAGE_READWRITE|PAGE_NOCACHE|PAGE_PHYSICAL);
	}

	void(*relmemcpy)(char*,char*,int);
	relmemcpy=(void (__cdecl *)(char *,char *,int))VirtualAlloc((void*)0x0, 1024, MEM_RESERVE|MEM_TOP_DOWN,PAGE_READWRITE);

	/* ask joshua */

	ret=VirtualCopy((void*)relmemcpy,(void *) (0xc0001000/256),	1024, PAGE_READWRITE|PAGE_NOCACHE|PAGE_PHYSICAL);

	if(!kernel_copy2) return;


	UINT32 phys_addr;
	phys_addr=KERNELCOPY;


	intr=(UINT32*)VirtualAlloc((void*)0x0,0x100, MEM_RESERVE,PAGE_READWRITE);

	// Interrupt control registers
	ret=VirtualCopy((void*)intr,(void *) (ICIP/256), 0x100, PAGE_READWRITE|PAGE_NOCACHE|PAGE_PHYSICAL);

	intr[1]=0;


	char *data1,*data2;

	data1=(char*)malloc(len);

	char *initrd1=NULL;

	if(fd1) initrd1=(char*)malloc(initrdl);

	if(!data1) return;

	if(!ret) return;

	data2= (char*)do_it;


	fread(data1,len,1,fd);


	if(fd1)
	{
		fread(initrd1,initrdl,1,fd1);
	}

//	ResetDevices();

	UART_puts("LinExec: Passing the point of no return.. Now.\r\n");

	UINT32	crc=0;

	/* TODO: Might need a config variable for this 64 too */
	setup_linux_params(BOOTIMG, INITRD,initrdl, 64*1024*1024 , param,kernel_copy2);

	memcpy(relmemcpy,mymemcpy,1024);
	relmemcpy(kernel_copy2,data2,0x100);

	if(fd1)
		relmemcpy(initrd_copy2,initrd1,initrdl);

	relmemcpy(kernel_copy2+0x8000,data1,len);

	
	DRAMloader(phys_addr, MACH_TYPE);
}

void mymemcpy(char* a, char* b, int size)
{
	while (size)
	{
		*a=*b;
		size--;
		a++; b++;
	};
};

/*
	Loads parameters from file given.
	The file has to be:
	kernel image
	initrd
	kernel cmdline
*/

void load_boot(char *ParamFile)
{
	FILE *stream;

	/* Tiny trick to enable the serial port, I couldn't enable the UART
	 * thru the registers */
	HANDLE com;
	com = CreateFile(L"COM1:", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

	stream=fopen(ParamFile,"r");
	if(!stream) return;
	char cmd[200],image[50],initrd[50];

	fgets(image,50,stream);
	image[strlen(image)-1]=0; // remove \n from the end
	
	fgets(initrd,50,stream);
	initrd[strlen(initrd)-1]=0;
	
	fgets(cmd,200,stream);
	cmd[strlen(cmd)-1]=0;

	UART_puts("LinExec: Beginning boot_linux.\r\n");
	boot_linux(image,initrd,cmd);
}	

/* wince programming was never easier */
int main(int argc, char **argv)
{ 
	printf("Hello world: %s\n",argv[0]);
	load_boot(argv[1]);
 return 0;
}
