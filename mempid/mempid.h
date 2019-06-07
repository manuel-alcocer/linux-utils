#ifndef _MEMPID_H_
#define _MEMPID_H_

#define PROCDIR "/proc"

enum _flags { SUMMARY = 1, PPID = 2 };

void help();

int * getpidlist(int ppid);

int isdir(const char *dirname);

#endif
