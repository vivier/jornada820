#include <asm/arch/hardware.h>

#undef readl
#define readl(a) (*(volatile unsigned int *)(SA1101_p2v(_Revision+0x100*(int)(a))))
#undef writel
#define writel(d, a) ((*(volatile unsigned int *)SA1101_p2v(_Revision+0x100*(int)(a)))=(d))

