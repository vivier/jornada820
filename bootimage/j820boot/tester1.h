
#include "config.h"

/////////////////////////////Functions /////////////////////////////////////

void Gpio();
void UART_setup();
void DumpMMU();
void UART_puts(char *);

void boot_linux(char*,char*);
UINT32 ReadPhysical(UINT32);
void  WritePhysical(UINT32 addr,UINT32 val);
UINT32 VirtualToPhysical(UINT32);

void load_boot(char*);
void SetGPIOalt(int,int);
void SetGPIOio(int,int);


////////////////////////////////////////////////////////////////////////////

extern "C" BOOL VirtualCopy(LPVOID lpvDestMem, LPVOID lpvSrcMem, 
							DWORD dwSizeInBytes, DWORD dwProtectFlag);

extern void do_it();

extern int read_mmu();		// reads where is/are descriptors located

extern void IntOff();
extern void DRAMloader(UINT32 adr,UINT32 machine_num); // this function turns off MMU and jumps onto physical address given

extern "C" DWORD SetProcPermissions(
DWORD newperms 
); 

extern "C" DWORD GetCurrentPermissions(
);

extern "C" BOOL SetKMode( 
BOOL fMode 
);

extern "C" LPVOID CreateStaticMapping(
DWORD dwPhysBase,
DWORD dwSize
);
