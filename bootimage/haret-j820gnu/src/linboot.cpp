/*
    Linux loader for Windows CE
    Copyright (C) 2003 Andrew Zabolotny

    For conditions of use see file COPYING
*/

#include <stdio.h>

#include "haret.h"
#include "xtypes.h"
#define CONFIG_ACCEPT_GPL
#include "setup.h"
#include "memory.h"
#include "util.h"
#include "output.h"
#include "gpio.h"
#include "video.h"
#include "cpu.h"
#include "resource.h"

extern "C" int LockPages(void *,int32,uint32 *,int32);
extern "C" int UnlockPages(void *,int32);


// Kernel file name
char *bootKernel = "zimage";
// Initrd file name
char *bootInitrd = "initrd";
// Kernel command line
char *bootCmdline = "root=/dev/ram0 ro console=tty0";
// Milliseconds to sleep for nicer animation :-)
uint32 bootSpeed = 5;
// ARM machine type (see linux/arch/arm/tools/mach-types)
uint32 bootMachineType = 0;

// Our own assembly functions
extern "C" uint32 linux_start (uint32 MachType, uint32 NumPages,
  uint32 KernelPA, uint32 PreloaderPA, uint32 TagKernelNumPages);
extern "C" void linux_preloader ();
extern "C" void linux_preloader_end ();

/* Set up kernel parameters. ARM/Linux kernel uses a series of tags,
 * every tag describe some aspect of the machine it is booting on.
 */
static void setup_linux_params (uint8 *tagaddr, uint32 initrd, uint32 initrd_size)
{
  struct tag *tag;

  tag = (struct tag *)tagaddr;

  // Core tag
  tag->hdr.tag = ATAG_CORE;
  tag->hdr.size = tag_size (tag_core);
  tag->u.core.flags = 0;
  tag->u.core.pagesize = 0x00001000;
  tag->u.core.rootdev = 0x0000; // not used, use kernel cmdline for this
  tag = tag_next (tag);

  // now the cmdline tag
  tag->hdr.tag = ATAG_CMDLINE;
  // tag header, zero-terminated string and round size to 32-bit words
  tag->hdr.size = (sizeof (struct tag_header) + strlen (bootCmdline) + 1 + 3) >> 2;
  strcpy (tag->u.cmdline.cmdline, bootCmdline);
  tag = tag_next (tag);

  // now the mem32 tag
  tag->hdr.tag = ATAG_MEM;
  tag->hdr.size = tag_size (tag_mem32);
  tag->u.mem.start = memPhysAddr;
  tag->u.mem.size = memPhysSize;
  tag = tag_next (tag);

  /* and now the initrd tag */
  if (initrd_size)
  {
    tag->hdr.tag = ATAG_INITRD2;
    tag->hdr.size = tag_size (tag_initrd);
    tag->u.initrd.start = initrd;
    tag->u.initrd.size = initrd_size;
    tag = tag_next (tag);
  }

  // now the NULL tag
  tag->hdr.tag = ATAG_NONE;
  tag->hdr.size = 0;
}

/* Loading process:
 * function do_it is loaded onto address ADDR_KERNEL along with parameters
 * (offset=0x100) and kernel image (offset=0x8000). Afterwards DRAMloader
 * is called; it disables MMU and jumps onto ADDR_KERNE. Function do_it
 * then copies kernel image to its proper address (0xA0008000 for xscale) and calls it.
 * Initrd is loaded onto address INITRD and the address is passed to kernel
 * via ATAG.
 */

#if XSCALE
void ResetDMA (pxaDMA *dma)
{
  int i;
  // Disable DMA interrupts
  dma->DINT = 0;
  // Clear DDADRx
  for (i = 0; i < 16; i++)
  {
    dma->Desc [i].DDADR = DDADR_STOP;
    dma->Desc [i].DCMD = 0;
  }
  // Set DMAs to Stop state
  for (i = 0; i < 16; i++)
    dma->DCSR [i] = DCSR_NODESC | DCSR_ENDINTR | DCSR_STARTINTR | DCSR_BUSERR;
  // Clear DMA requests to channel map registers (just in case)
  for(i = 0; i < 40; i ++)
    dma->DRCMR [i] = 0;
}

void ResetUDC (pxaUDC *udc)
{
  udc->UDCCS [ 2] = UDCCS_BO_RPC | UDCCS_BO_SST;
  udc->UDCCS [ 7] = UDCCS_BO_RPC | UDCCS_BO_SST;
  udc->UDCCS [12] = UDCCS_BO_RPC | UDCCS_BO_SST;
  udc->UDCCS [ 1] = UDCCS_BI_TPC | UDCCS_BI_FTF |
    UDCCS_BI_TUR | UDCCS_BI_SST | UDCCS_BI_TSP;
  udc->UDCCS [ 6] = UDCCS_BI_TPC | UDCCS_BI_FTF |
    UDCCS_BI_TUR | UDCCS_BI_SST | UDCCS_BI_TSP;
  udc->UDCCS [11] = UDCCS_BI_TPC | UDCCS_BI_FTF |
    UDCCS_BI_TUR | UDCCS_BI_SST | UDCCS_BI_TSP;
  udc->UDCCS [ 3] = UDCCS_II_TPC | UDCCS_II_FTF |
    UDCCS_II_TUR | UDCCS_II_TSP;
  udc->UDCCS [ 8] = UDCCS_II_TPC | UDCCS_II_FTF |
    UDCCS_II_TUR | UDCCS_II_TSP;
  udc->UDCCS [13] = UDCCS_II_TPC | UDCCS_II_FTF |
    UDCCS_II_TUR | UDCCS_II_TSP;
  udc->UDCCS [ 4] = UDCCS_IO_RPC | UDCCS_IO_ROF;
  udc->UDCCS [ 9] = UDCCS_IO_RPC | UDCCS_IO_ROF;
  udc->UDCCS [11] = UDCCS_IO_RPC | UDCCS_IO_ROF;
  udc->UDCCS [ 5] = UDCCS_INT_TPC | UDCCS_INT_FTF |
    UDCCS_INT_TUR | UDCCS_INT_SST;
  udc->UDCCS [10] = UDCCS_INT_TPC | UDCCS_INT_FTF |
    UDCCS_INT_TUR | UDCCS_INT_SST;
  udc->UDCCS [15] = UDCCS_INT_TPC | UDCCS_INT_FTF |
    UDCCS_INT_TUR | UDCCS_INT_SST;
}
#else

void Reset_SA1100_DMA ()
{
  int i;

#if 0
  // Disable DMA interrupts
  dma->DINT = 0;


  // Clear DDADRx
  for (i = 0; i < 16; i++)
  {
    dma->Desc [i].DDADR = DDADR_STOP;
    dma->Desc [i].DCMD = 0;
  }
  // Set DMAs to Stop state
  for (i = 0; i < 16; i++)
    dma->DCSR [i] = DCSR_NODESC | DCSR_ENDINTR | DCSR_STARTINTR | DCSR_BUSERR;
  // Clear DMA requests to channel map registers (just in case)
  for(i = 0; i < 40; i ++)
    dma->DRCMR [i] = 0;
#endif
}

void Reset_SA1101_USB ()
{

}

#endif

// Whew... a real Microsoft API function (by number of parameters :)
static bool read_file (FILE *f, uint8 *buff, uint32 size, uint32 totsize,
                       uint32 &totread, videoBitmap &thermored, int dx, int dy)
{
  uint8 *cur = buff;
  uint32 th = thermored.GetHeight ();
  uint sy1 = (totread * th) / totsize;
  while (size)
  {
    int c = size;
    if (c > 16 * 1024)
      c = 16 * 1024;
    c = fread (cur, 1, c, f);
    if (c <= 0)
      return false;

    size -= c;
    cur += c;

    totread += c;
    uint sy2 = (totread * th) / totsize;

    while (sy1 < sy2)
    {
#if XSCALE
      thermored.DrawLine (dx + THERMOMETER_X, dy + THERMOMETER_Y +
                          th - 1 - sy1, sy1);
#endif
      sy1++;
      Sleep (bootSpeed);
    }
  }

  return true;
}

/* LINUX BOOT PROCESS
 *
 * Since we need to load Linux at the beginning of physical RAM, it will
 * overwrite the MMU L1 table which is located in WinCE somewhere around
 * there. Thus we have either to copy the kernel and initrd twice (once
 * to some well-known location and second time to the actual destination
 * address (0xa0008000 for XScale), or to use a clever algorithm (which
 * is used here):
 *
 * A "kernel bundle" is prepared. It consists of the following things:
 * - Several values to be further passed to kernel (such as machine type).
 * - The tag list to be passed to kernel.
 * - The kernel
 * - The initrd
 *
 * Then we proceed as follows: create a one-to-one virtual:physical mapping,
 * then copy to the very end of physical RAM (where there's little chance of
 * overwriting something important) a little pre-loader along with a large
 * table containing a list of physical addresses for every 4k page of the
 * kernel bundle.
 *
 * Now when the pre-loader gets control, it disables MMU and starts copying
 * kernel bundle to desired location (since it knows the physical location).
 * Finally, it passes control to kernel.
 */
void bootLinux ()
{
  char fn [200];
  uint32	kmode;
  
  fnprepare (bootKernel, fn, sizeof (fn) / sizeof (wchar_t));
  FILE *fk = fopen (fn, "rb");
  if (!fk)
  {
    Complain (C_ERROR ("Failed to load kernel %hs"), fn);
    return;
  }

  uint32 i, ksize, isize = 0;

  // Find out kernel image size
  fseek (fk, 0, SEEK_END);
  ksize = ftell (fk);
  Output (L"kernel size: %d bytes, needs %d pages", ksize,(ksize + 4095) & ~4095);

  fseek (fk, 0, SEEK_SET);

  FILE *fi;
  if (bootInitrd && *bootInitrd)
  {
    fnprepare (bootInitrd, fn, sizeof (fn) / sizeof (wchar_t));
    fi = fopen (fn, "rb");
    if (fi)
    {
      fseek (fi, 0, SEEK_END);
      isize = ftell (fi);
      Output (L"initrd size: %d bytes, needs %d pages", isize,(isize + 4095) & ~4095);
      fseek (fi, 0, SEEK_SET);
    }
  }

#if XSCALE
  // Load the bitmaps used to display load progress
  videoBitmap logo (IDB_LOGO);
  videoBitmap thermored (IDB_THERMORED);
  videoBitmap thermoblue (IDB_THERMOBLUE);
  videoBitmap eyes (IDB_EYES);

  videoBeginDraw ();

  int dx = (videoW - logo.GetWidth ()) / 2;
  int dy = (videoH - logo.GetHeight ()) / 2;

  logo.Draw (dx, dy);
  thermoblue.Draw (dx + THERMOMETER_X, dy + THERMOMETER_Y);
#else
  videoBitmap thermored (IDB_THERMORED);
#endif

  // Align kernel and initrd sizes to nearest 4k boundary.
  uint totsize = ksize + isize;
  uint aksize = (ksize + 4095) & ~4095;
  Output (L"aligned kernel size: %d bytes, needs %d pages", aksize,(aksize + 4095) & ~4095);
  uint aisize = (isize + 4095) & ~4095;
  Output (L"aligned initrd size: %d bytes, needs %d pages", aisize,(aisize + 4095) & ~4095);
  // Construct kernel bundle (tags + kernel + initrd)
  uint8 *kernel_bundle;
  uint kbsize = 4096 + aksize + aisize;
  Output (L"bundle size: %d bytes, needs %d pages", kbsize,(kbsize + 4095) & ~4095);

  // We have to ensure that physical memory allocated kernel is not
  // located too low. If we won't do this it could happen that we'll overlap
  // memory locations when copying from the allocated buffer to destination
  // physical address.
  uint alloc_tries = 10;
#if XSCALE
  uint kernel_addr = memPhysAddr + 0x8000;
#else
  uint kernel_addr = memPhysAddr + 0x8000 + 0x00200000;
#endif

  Output (L"physical kernel address: %08x", kernel_addr);

#if 0
#if 
  GarbageCollector gc;

  for (;;)
  {
#if 0
    uint32 mem = (uint32) malloc (kbsize + 4095);
#else
   uint32 mem = (uint32 )VirtualAlloc (NULL,kbsize + 4095,MEM_COMMIT,PAGE_READWRITE);
   uint32 *mema = (uint32 *) malloc ((kbsize + 4095)/4);

   int ret1=LockPages((void *)mem,kbsize + 4095,mema,1);
   if (!ret1)
   {
    Output (L"LockPages failed: npages=%d",(kbsize + 4095)/4096);
    return;
   }
#endif
    if (!mem)
    {
      Output (L"FATAL: Not enough memory for kernel bundle!");
      gc.FreeAll ();
      return;
    }
#endif

    // Kernel bundle should start at the beginning of page
    kernel_bundle = (uint8 *)((mem + 4095) & ~0xfff);
    // Since we copy the kernel to the beginning of physical memory page
    // by page it will be enough if we ensure that every allocated page has
    // physical address larger than its final physical address.
    bool ok = true;
    for (i = 0; i <= (kbsize >> 12); i++)
      if (memVirtToPhys ((uint32)kernel_bundle + (i << 12)) < kernel_addr + (i << 12))
      {
        ok = false;
        break;
      }

    if (ok)
      break;

    gc.Collect ((void *)mem);
    Output (L"WARNING: page %d has addr %08x, target addr %08x, retrying",
            i, memVirtToPhys ((uint32)kernel_bundle + (i << 12)),
            kernel_addr + (i << 12));

    if (!--alloc_tries)
    {
      Output (L"FATAL: Cannot allocate kernel bundle in high memory!");
      gc.FreeAll ();
      return;
    }
  }

  gc.FreeAll ();
#else
#endif

  uint8 *taglist = kernel_bundle;
  uint8 *kernel = taglist + 4096;
  uint8 *initrd = kernel + aksize;

  // Now read the kernel and initrd
  uint totread = 0;

#if XSCALE
  if (!read_file (fk, kernel, ksize, totsize, totread, thermored, dx, dy))
#else
  if (!read_file (fk, kernel, ksize, totsize, totread, thermored, 0, 0))
#endif
  {
    Complain (C_ERROR ("Error reading kernel `%hs'"), bootKernel);
errexit:
    free (kernel);
    return;
  }
  fclose (fk);

  if (isize)
  {
#if XSCALE
    if (!read_file (fi, initrd, isize, totsize, totread, thermored, dx, dy))
#else
    if (!read_file (fi, initrd, isize, totsize, totread, thermored, 0, 0))
#endif
    {
      Complain (C_ERROR ("Error reading initrd `%hs'"), bootInitrd);
      goto errexit;
    }
    fclose (fi);
  }

  // Now construct our "page table" which will work in absense of MMU
  uint npages = (4096 + aksize + aisize) >> 12;
  uint preloader_size = (0x100 + npages * 4 + 0xfff) & ~0xfff;

  uint8 *preloader;
  uint32 preloaderPA;
  uint32 *ptable;

  // We need the preloader to be contiguous in physical memory.
  // It's not so big (usually one or two 4K pages), however we may
  // need to allocate it a couple of times till we get it as we need it...
  // Also, naturally, preloader cannot be located in the memory where
  // kernel will be copied.

  alloc_tries = 10;
  for (;;)
  {
#if 0
    uint32 mem = (uint32)malloc (preloader_size + 4095);
#else
    uint32 *mem = (uint32 *)VirtualAlloc (NULL,preloader_size + 4095,MEM_COMMIT,PAGE_READWRITE);
   uint32 *mema = (uint32 *) malloc ((preloader_size + 4095)/4);
 
   int ret1=LockPages((void *)mem,preloader_size + 4095,NULL,1);
   if (!ret1)
   {
    Output (L"LockPages failed: npages=%d",(preloader_size + 4095)/4096);
    return;
   }
#endif
#if 0
    if (!mem)
    {
      Output (L"FATAL: Not enough memory for preloader!");
      gc.FreeAll ();
      return;
    }
#endif
//    preloader = (uint8 *)((mem + 0xfff) & ~0xfff);
    preloader = (uint8 *)mem;
    preloaderPA = memVirtToPhys ((uint32)preloader);

  Output (L"TMP Preloader physical/virtual address: %08x %08x %08x", mema[0], preloaderPA, preloader);

    bool ok = true;
    uint32 minaddr = memPhysAddr + 0x7000 + 0x200000 + kbsize;
    for (i = 1; i < (preloader_size >> 12); i++)
    {
      uint32 pa = memVirtToPhys ((uint32)preloader + (i << 12));
      if ((pa != (preloaderPA + (i << 12))) || (pa < minaddr))
      {
        ok = false;
        break;
      }
    }

    if (ok)
      break;

//    gc.Collect ((void *)mem);

    if (!--alloc_tries)
    {
      Output (L"FATAL: Cannot allocate a contiguous physical memory area!");
//      gc.FreeAll ();
      return;
    }
  }

//  gc.FreeAll ();

  Output (L"Preloader physical/virtual address: %08x %08x", preloaderPA, preloader);

  memcpy (preloader, (const void *)&linux_preloader,
          (uint32)&linux_preloader_end - (uint32)&linux_preloader);
  ptable = (uint32 *)(preloader + 0x100);

  for (i = 0; i < npages; i++)
    ptable [i] = memVirtToPhys ((uint32)kernel_bundle + (i << 12));

  // Recommended kernel placement = RAM start + 32K
  // Initrd will be put at the address of kernel + 4Mb
  // (let's hope uncompressed kernel never happens to be larger than that).
#if XSCALE
  uint32 initrd_phys_addr = memPhysAddr + 0x8000 + 0x400000;
#else
  uint32 initrd_phys_addr = memPhysAddr + 0x8000 + 0x200000 + 0x400000;
#endif
  if (isize)
    Output (L"Physical initrd address: %08x", initrd_phys_addr);

  setup_linux_params (taglist, initrd_phys_addr, isize);

  Output (L"goodbye wince ...");
  Sleep (500);

#if XSCALE
  // Reset AC97
  memPhysWrite (0x4050000C,0);
#else
#endif

  /* Map now everything we'll need later */
  uint32 old_icmr;
  uint32 *icmr = (uint32 *)memPhysMap (ICMR);
  uint32 *mmu = (uint32 *)memPhysMap (cpuGetMMU ());

#if XSCALE
  pxaDMA *dma = (pxaDMA *)memPhysMap (0x40000000);
  pxaUDC *udc = (pxaUDC *)memPhysMap (UDC_BASE_ADDR);
#else
#endif

#if XSCALE
  eyes.Draw (dx + PENGUIN_EYES_X, dy + PENGUIN_EYES_Y);
#else
#endif

  /* Set thread priority to maximum */
  SetThreadPriority (GetCurrentThread (), THREAD_PRIORITY_TIME_CRITICAL);

#if _WIN32_WCE >= 0x300
  /* Disable multitasking (heh) */
  SetThreadQuantum (GetCurrentThread (), 0);
#endif

  Output (L"old permissions: %x pid=%d",GetCurrentPermissions(),cpuGetPID());
  /* Allow current process to access any memory domains */
  SetProcPermissions (0xffffffff);
  Output (L"new permissions: %x pid=%d",GetCurrentPermissions(),cpuGetPID());

  /* Go into kernel mode. */
  Output (L"kernel mode1: %hs",SetKMode(TRUE)?"yes":"no");
  Output (L"kernel mode2: %hs",(kmode=SetKMode(TRUE))?"yes":"no");
  if (kmode == 0)
  {
   Output (L"kmode switch failed");
   return;
  }

// no more Output.
  Output (L"disabling interrupts");
  cli ();

  old_icmr = *icmr;
  *icmr = 0;

#if XSCALE
  ResetDMA (dma);
  ResetUDC (udc);
#else
  Reset_SA1100_DMA ();
  Reset_SA1101_USB ();
#endif

  __try
  {
    //cpuSetDACR (0xffffffff);

    // Create the virtual->physical 1:1 mapping entry in
    // 1st level descriptor table. These addresses are hopefully
    // unused by WindowsCE (or rather unimportant for us now).
    cpuFlushCache ();
    mmu [preloaderPA >> 20] = (preloaderPA & MMU_L1_SECTION_MASK) |
        MMU_L1_SECTION | MMU_L1_AP_MASK;

    // Penguinize!
    linux_start (bootMachineType, npages, memPhysAddr, preloaderPA,
                 1 + (aksize >> 12));
  }
  // We should never get here, but if we crash try to recover ...
#if 0
  __except (EXCEPTION_EXECUTE_HANDLER)
  {
    // UnresetDevices???
    *icmr = old_icmr;
    sti ();
    SetKMode (FALSE);
#if XSCALE
    videoEndDraw ();
#endif
    Output (L"Linux boot failed because of a exception!");
  };
#endif
}
