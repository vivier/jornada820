/*
    Periferial I/O devices interface
    Copyright (C) 2003 Andrew Zabolotny

    For conditions of use see file COPYING
*/

#include <time.h>
#include <wtypes.h>

#include "xtypes.h"
#include "gpio.h"
#include "memory.h"
#include "output.h"

#ifndef SA1100
#define GPIO_MAX 84
#endif

// Which GPIO changes to ignore during watch
uint32 gpioIgnore [1] = {0};

void gpioSetDir (int num, bool out)
{
  if (num > GPIO_MAX)
    return;

  uint32 *gpdr = (uint32 *)memPhysMap (GPDR);

  uint32 mask = 1 << (num & 31);

  if (out)
    gpdr [0] |= mask;
  else
    gpdr [0] &= ~mask;
}

bool gpioGetDir (int num)
{
  if (num > GPIO_MAX)
    return false;

  uint32 *gpdr = (uint32 *)memPhysMap (GPDR);

  return (gpdr [0] & (1 << (num & 31))) != 0;
}

void gpioSetAlt (int num, int altfn)
{
  if (num > GPIO_MAX)
    return;

  uint32 *gafr = (uint32 *)memPhysMap (GAFR);

  gafr [0] = (gafr [0] & ~(1 << (num & 31)));
}

uint32 gpioGetAlt (int num)
{
  if (num > GPIO_MAX)
    return 0xffffffff;

  uint32 *gafr = (uint32 *)memPhysMap (GAFR);

  return (gafr [0] >> (num & 31)) & 1;
}

int gpioGetState (int num)
{
  if (num > GPIO_MAX)
    return -1;

  uint32 *gplr = (uint32 *)memPhysMap (GPLR);
  return (gplr [0] >> (num & 31)) & 1;
}

void gpioSetState (int num, bool state)
{
  if (num > GPIO_MAX)
    return;

  uint32 *gpscr = (uint32 *)memPhysMap (state ? GPSR : GPCR);
  gpscr [num >> 5] |= 1 << (num & 31);
}

int gpioGetSleepState (int num)
{
  if (num > GPIO_MAX)
    return -1;

  uint32 *pgsr = (uint32 *)memPhysMap (PGSR);
  return (pgsr [num >> 5] >> (num & 31)) & 1;
}

void gpioSetSleepState (int num, bool state)
{
  if (num > GPIO_MAX)
    return;

  uint32 *pgsr = (uint32 *)memPhysMap (PGSR);
  uint32 ofs = num >> 5;
  uint32 mask = 1 << (num & 31);

  if (state)
    pgsr [ofs] |= mask;
  else
    pgsr [ofs] &= ~mask;
}

void gpioWatch (uint seconds)
{
  if (seconds > 300)
  {
    Complain (L"Number of seconds trimmed to 300");
    seconds = 300;
  }

  int cur_time = time (NULL);
  int fin_time = cur_time + seconds;
  int i, j;

  uint32 *gplr = (uint32 *)memPhysMap (GPLR);
  uint32 old_gplr [1];
  for (i = 0; i < 1; i++)
    old_gplr [i] = gplr [i];

  uint32 *gpdr = (uint32 *)memPhysMap (GPDR);
  uint32 old_gpdr [1];
  for (i = 0; i < 1; i++)
    old_gpdr [i] = gpdr [i];

  uint32 *gafr = (uint32 *)memPhysMap (GAFR);
  uint32 old_gafr [1];
  for (i = 0; i < 1; i++)
    old_gafr [i] = gafr [i];

  while (cur_time <= fin_time)
  {
    gplr = (uint32 *)memPhysMap (GPLR);
    for (i = 0; i < 1; i++)
    {
      uint32 val = gplr [i];
      if (old_gplr [i] != val)
      {
        uint32 changes = (old_gplr [i] ^ val) & ~gpioIgnore [i];
        for (j = 0; j < 32; j++)
          if ((changes & (1 << j))
	   && ((i * 32 + j) <= GPIO_MAX))
	  {
            Output (L"GPLR[%d] changed to %d", i * 32 + j, (val >> j) & 1);
            Log (L"GPLR[%d] changed to %d", i * 32 + j, (val >> j) & 1);
          }
        old_gplr [i] = val;
      }
    }

    gpdr = (uint32 *)memPhysMap (GPDR);
    for (i = 0; i < 1; i++)
    {
      uint32 val = gpdr [i];
      if (old_gpdr [i] != val)
      {
        uint32 changes = (old_gpdr [i] ^ val) & ~gpioIgnore [i];
        for (j = 0; j < 32; j++)
          if ((changes & (1 << j))
	   && ((i * 32 + j) <= GPIO_MAX))
	  {
            Output (L"GPDR[%d] changed to %d", i * 32 + j, (val >> j) & 1);
            Log (L"GPDR[%d] changed to %d", i * 32 + j, (val >> j) & 1);
	  }
        old_gpdr [i] = val;
      }
    }

    gafr = (uint32 *)memPhysMap (GAFR);
    for (i = 0; i < 1; i++)
    {
      uint32 val = gafr [i];
      if (old_gafr [i] != val)
      {
        uint32 changes = old_gafr [i] ^ val;
        for (j = 0; j < 32; j++)
          if (changes & (1 << j)
	   && ((i * 32 + j) <= (GPIO_MAX)))
	  {
            Output (L"GAFR[%d] changed to %d", i * 32 + j, (changes >> j ) & 1);
            Log (L"GAFR[%d] changed to %d", i * 32 + j, (changes >> j ) & 1);
	  }
        old_gafr [i] = val;
      }
    }

    cur_time = time (NULL);
  }
}

uint32 gpioScrGPLR (bool setval, uint32 *args, uint32 val)
{
  if (args [0] > GPIO_MAX)
  {
    Output (L"Valid GPIO indexes are 0..%d, not %d", args [0], GPIO_MAX);
    return 0xffffffff;
  }

  if (setval)
  {
    gpioSetState (args [0], val != 0);
    return 0;
  }

  return gpioGetState (args [0]);
}

uint32 gpioScrGPDR (bool setval, uint32 *args, uint32 val)
{
  if (args [0] > GPIO_MAX)
  {
    Output (L"Valid GPIO indexes are 0..%d, not %d", args [0],GPIO_MAX);
    return 0xffffffff;
  }

  if (setval)
  {
    gpioSetDir (args [0], val != 0);
    return 0;
  }

  return gpioGetDir (args [0]);
}

uint32 gpioScrGAFR (bool setval, uint32 *args, uint32 val)
{
  if (args [0] > GPIO_MAX)
  {
    Output (L"Valid GPIO indexes are 0..%d, not %d", args [0],GPIO_MAX);
    return 0xffffffff;
  }

  if (setval)
  {
    gpioSetAlt (args [0], val);
    return 0;
  }

  return gpioGetAlt (args [0]);
}

bool gpioDump (void (*out) (void *data, const char *, ...),
               void *data, uint32 *args)
{
  const uint rows = (GPIO_MAX/4)+1;
  uint32 *grer = (uint32 *)memPhysMap (GRER);
  uint32 *gfer = (uint32 *)memPhysMap (GFER);

  out (data, "GPIO# D S A INTER | GPIO# D S A INTER | GPIO# D S A INTER | GPIO# D S A INTER\n");
  out (data, "------------------+-------------------+-------------------+------------------\n");
  for (uint i = 0; i < rows; i++)
  {
    for (uint j = 0; j < 4; j++)
    {
      uint gpio = i + j * rows;
      bool re = (grer [gpio >> 5] & (1 << (gpio & 31))) != 0;
      bool fe = (gfer [gpio >> 5] & (1 << (gpio & 31))) != 0;

      out (data, "%3d   %c %d %d %s%s%s", gpio, gpioGetDir (gpio) ? 'O' : 'I',
           gpioGetState (gpio), gpioGetAlt (gpio),
           re ? "RE " : "   ", fe ? "FE" : "  ",
           j < 3 ? " | " : "\n");
    }
  }
  return true;
}

bool gpioDumpState (void (*out) (void *data, const char *, ...),
                    void *data, uint32 *args)
{
  int i;

#if XSCALE
  out (data, "/* GPIO pin direction setup */\n");
  for (i = 0; i < GPIO_MAX-3; i++)
    out (data, "#define GPIO%02d_Dir\t%d\n",
         i, gpioGetDir (i));
  out (data, "\n/* GPIO Alternate Function (Select Function 0 ~ 3) */\n");
  for (i = 0; i < GPIO_MAX-3; i++)
    out (data, "#define GPIO%02d_AltFunc\t%d\n",
         i, gpioGetAlt (i));
  out (data, "\n/* GPIO Pin Init State */\n");
  for (i = 0; i < GPIO_MAX-3; i++)
    out (data, "#define GPIO%02d_Level\t%d\n",
         i, gpioGetDir (i) ? gpioGetState (i) : 0);
  out (data, "\n/* GPIO Pin Sleep Level */\n");
  for (i = 0; i < GPIO_MAX-3; i++)
    out (data, "#define GPIO%02d_Sleep_Level\t%d\n",
         i, gpioGetSleepState (i));
#else
  out (data, "/* GPIO pin direction setup */\n");
  for (i = 0; i < GPIO_MAX-3; i++)
    out (data, "#define GPIO%02d_Dir\t%d\n",
         i, gpioGetDir (i));
  out (data, "\n/* GPIO Alternate Function (Select Function 0 ~ 3) */\n");
  for (i = 0; i < GPIO_MAX-3; i++)
    out (data, "#define GPIO%02d_AltFunc\t%d\n",
         i, gpioGetAlt (i));
  out (data, "\n/* GPIO Pin Init State */\n");
  for (i = 0; i < GPIO_MAX-3; i++)
    out (data, "#define GPIO%02d_Level\t%d\n",
         i, gpioGetDir (i) ? gpioGetState (i) : 0);
  out (data, "\n/* GPIO Pin Sleep Level */\n");
  for (i = 0; i < GPIO_MAX-3; i++)
    out (data, "#define GPIO%02d_Sleep_Level\t%d\n",
         i, gpioGetSleepState (i));
#endif
  return true;
}


static uint WATCH;
static uint32 old_reg;

void do_addrWatch(HWND hwnd,UINT uMsg,UINT idEvent,DWORD dwTime);
void addrWatch (uint addr)
{
//  if (seconds > 60)
//  {
//    Complain (L"Number of seconds trimmed to 60");
uint    seconds = 60;
//  }


  int cur_time = time (NULL);
  int fin_time = cur_time + seconds;

volatile  uint32 reg;

  WATCH=addr;

  reg = (uint32)memPhysMap (WATCH);

    Output (L"W=%08x %d",*(uint32 *)reg,GetTickCount());

#if 0
  volatile uint32 old_reg, val;
  int j;
    
  old_reg = *(uint32 *)reg;

  while (cur_time <= fin_time)
  {
      val=*(uint32 *)reg;
      if (old_reg != val)
      {
        uint32 changes = (old_reg ^ val);
        for (j = 0; j < 32; j++)
          if ((changes & (1 << j)) )
	  {
            Output (L"B%d %d", j, (val >> j) & 1);
          }
        old_reg = val;
      }
    cur_time = time (NULL);
  }
#else
    Output (L"before settimer %d",GetTickCount());
 if (!SetTimer(NULL,130,1000, (TIMERPROC)do_addrWatch))
    Output (L"failed settimer %d",GetTickCount());
    Sleep(30);
 
    Output (L"after settimer %d",GetTickCount());
#endif
//  reg = (uint32)memPhysMap (WATCH);
    Output (L"W=%08x %d",*(uint32 *)reg,GetTickCount());


}

void do_addrWatch(  HWND hwnd,     UINT uMsg,       UINT idEvent,         DWORD dwTime 
        )
{

  int j;

volatile  uint32 reg;

  reg = (uint32)memPhysMap (WATCH);

//    Output (L"W=%08x %d",*(uint32 *)reg,GetTickCount());

//  reg = (uint32)memPhysMap (WATCH);
      volatile uint32 val;

    Output (L"in settimer in: %d",GetTickCount());
    
      val=*(uint32 *)reg;
      if (old_reg != val)
      {
        uint32 changes = (old_reg ^ val);
        for (j = 0; j < 32; j++)
          if ((changes & (1 << j)) )
	  {
            Output (L"B%d %d", j, (val >> j) & 1);
          }
        old_reg = val;
      }

    Output (L"in settimer out: %d",GetTickCount());
}
