/*
    Utility routines
    Copyright (C) 2003 Andrew Zabolotny

    For conditions of use see file COPYING
*/

#ifndef _UTIL_H
#define _UTIL_H

// strchr() for wide chars
extern wchar_t *wstrchr (wchar_t *s, wchar_t c);
// strlen() for wide chars
extern int wstrlen (wchar_t *s);
// Create a duplicate of string with new char []
extern char *strnew (const char *s);
// Prepend executable source directory to file name if it does not
// already contain a path.
extern void fnprepare (const char *ifn, char *ofn, int ofn_max);

#if 0
// A very simple "garbage collector". In fact, it just tracks a number of
// pointers to be freed later.
class GarbageCollector
{
  void **Pointers;
  uint Count, Max;
public:
  GarbageCollector ();
  ~GarbageCollector ();
  void Collect (void *p);
  void FreeAll ();
};
#endif

extern wchar_t SourcePath [];

#endif /* _UTIL_H */
