#include "tester1.h"
#include "setup.h"

UINT32 ReadPhysical(UINT32 adr)
{
	UINT32 base=  adr&0xffff0000;
	UINT32 offset=adr&0x0000ffff;
	UINT32 *p=(UINT32*)VirtualAlloc(0,0x10000,MEM_RESERVE,PAGE_READWRITE);
	if(!p) return 0xFFFFFFFF;
	if(!VirtualCopy((void*)p,(void*)(base/256),0x10000,PAGE_READWRITE|PAGE_NOCACHE|PAGE_PHYSICAL)) return 0xFFFFFFFF;
	UINT32 val=p[offset/4];
	VirtualFree(p,0,MEM_RELEASE);
	return val;
}

void WritePhysical(UINT32 adr,UINT32 val)
{
	UINT32 base=  adr&0xffff0000;
	UINT32 offset=adr&0x0000ffff;
	UINT32 *p=(UINT32*)VirtualAlloc(0,0x10000,MEM_RESERVE,PAGE_READWRITE);
	VirtualCopy((void*)p,(void*)(base/256),0x10000,PAGE_READWRITE|PAGE_NOCACHE|PAGE_PHYSICAL);
	p[offset/4]=val;
	VirtualFree(p,0,MEM_RELEASE);
}

UINT32 VirtualToPhysical(UINT32 Virtual)
{
	FILE *log=fopen("\\logger1.txt","w");

	fprintf(log,"virtual: 0x%lx\n",Virtual);
	UINT32 mmu=(UINT32)read_mmu();
//	mmu=0xa0000000;
	fprintf(log,"mmu:  0x%lx \n",mmu);

	UINT32 AdrFirstLevDesc=(mmu&0xffffc000)+((Virtual>>18)&0xfffffffc);
	fprintf(log,"AdrFirstLevDesc:  0x%lx \n",AdrFirstLevDesc);

	UINT32 FirstLevDesc=ReadPhysical(AdrFirstLevDesc);
	fprintf(log,"FirstLevDesc:  0x%lx \n",FirstLevDesc);

	
	if(0)
	{
		fprintf(log,"Page\n");
		UINT32 PhysAddr=(FirstLevDesc&0xfff00000)+(Virtual&0xfffff);
		fprintf(log,"Physical address:  0x%lx \n",PhysAddr);
		fclose(log);
		return PhysAddr;
	}

	if(FirstLevDesc&0x3==3)	// tiny page
	{
		fprintf(log,"Tiny page\n");
		UINT32 AdrSecondLevDesc=(FirstLevDesc&0xfffff000)+((Virtual>>8)&0xffc);
		fprintf(log,"AdrSecondLevDesc:  0x%lx \n",AdrSecondLevDesc);

		UINT32 SecondLevDesc=ReadPhysical(AdrSecondLevDesc);
		fprintf(log,"SecondLevDesc:  0x%lx \n",SecondLevDesc);
	
		UINT32 PhysAddr=(SecondLevDesc&0xffffc000)+(Virtual&0x3ff);
		fprintf(log,"Physical address:  0x%lx \n",PhysAddr);
		fclose(log);
		return PhysAddr;
	}
//	if(FirstLevDesc&0x3==3)	// small page
	{
		fprintf(log,"Else page\n");
		UINT32 AdrSecondLevDesc=(FirstLevDesc&0xfffffc00)+((Virtual>>10)&0x03fc);
		fprintf(log,"AdrSecondLevDesc:  0x%lx \n",AdrSecondLevDesc);

		UINT32 SecondLevDesc=ReadPhysical(AdrSecondLevDesc);
		fprintf(log,"SecondLevDesc:  0x%lx \n",SecondLevDesc);

		UINT32 PhysAddr=(SecondLevDesc&0xffff0000)+(Virtual&0xffff);
		fprintf(log,"Physical address:  0x%lx \n",PhysAddr);
		fclose(log);
		return PhysAddr;

	}
}






void DumpMMU()
{
	void *mmu=(void*)(MEM_START);

	UINT32 *_mmu=(UINT32*)VirtualAlloc((void*)0x0,sizeof(void*)*0xffff, MEM_RESERVE,PAGE_READWRITE);
	int ret=VirtualCopy(_mmu,(void *) ((UINT32)mmu/256),sizeof(void*)*0xffff	, PAGE_READWRITE|PAGE_NOCACHE|PAGE_PHYSICAL);

	FILE *log=fopen("\\logger2.txt","w");
	fprintf(log,"mmu_table=0x%lx : \n",_mmu);
	fprintf(log,"mmu=0x%lx : \n",mmu);
	fprintf(log,"ret=0x%x : \n",ret);
	for(UINT32 z=0;z<=0x0100;z++)
		fprintf(log,"mmu_table[0x%x]=0x%lx: \n",z,_mmu[z]);
	fclose(log);
	return;
}
