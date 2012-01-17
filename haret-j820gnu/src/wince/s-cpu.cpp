/*
    CPU & coprocessor manipulations
    Copyright (C) 2003 Andrew Zabolotny

    For conditions of use see file COPYING
*/

#include <windows.h>

#include "xtypes.h"
#include "cpu.h"
#include "output.h"
#include "haret.h"

// Self-modified code
static uint32 selfmod [2] =
{
  0xee100010,	// mrc pX,0,r0,crX,cr0,0
  0xe1a0f00e    // mov pc,lr
};

static bool FlushSelfMod (const char *op)
{
  bool rc = true;
//  __try
  {
    SetKMode (TRUE);
    cli ();
    cpuFlushCache ();
    sti ();
    SetKMode (FALSE);
  }
#if 0
  __except (EXCEPTION_EXECUTE_HANDLER)
  {
    Complain (C_ERROR ("EXCEPTION while preparing to %hs coprocessor"), op);
    rc = false;
  }
#endif
  return rc;
}

uint32 cpuGetCP (uint cp, uint regno)
{
	uint32 result=0xffffffff;
	int ok=0;

      	if (cp > 15)
    return 0xffffffff;

if (cp==15)
{
    ok=1;
	SetKMode (TRUE);
    cli ();
switch (regno)
{
	case 0: 
	result=cp15_0();
       break;
	case 2:
       result=cp15_2();
       break;
	case 13:
       result=cp15_13();
       break;
	default:
       ok=0;
       break;
}
    sti ();
    SetKMode (FALSE);
    }

    if (!ok) Output (L"Invalid register read cp=%d regno=%d\n",cp,regno);

	  return result;
  /* here live the daemons */
	
	
  
  uint32 value;
  selfmod [0] = 0xee100010 | (cp << 8) | (regno << 16);

  if (!FlushSelfMod ("read"))
    return 0xffffffff;

//  __try
  {
    value = ((uint32 (*) ())&selfmod) ();
  }
#if 0
  __except (EXCEPTION_EXECUTE_HANDLER)
  {
    Complain (C_ERROR ("EXCEPTION reading coprocessor %d register %d"), cp, regno);
    value = 0xffffffff;
  }
#endif
  return value;
}

bool cpuSetCP (uint cp, uint regno, uint32 val)
{
  if (cp > 15)
    return false;

  selfmod [0] = 0xee000f10 | (cp << 8) | (regno << 16);
  if (!FlushSelfMod ("write"))
    return false;

  bool rc = true;
//  __try
  {
    ((void (*) (uint32))&selfmod) (val);
  }
#if 0 
 __except (EXCEPTION_EXECUTE_HANDLER)
  {
    Complain (C_ERROR ("EXCEPTION writing to coprocessor %d register %d"), cp, regno);
    rc = false;
  }
#endif

  return rc;
}
