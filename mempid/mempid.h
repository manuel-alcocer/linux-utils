#ifndef _MEMPID_H_
#define _MEMPID_H_

#define PROCDIR "/proc"
#define STATFILENAME "stat"

#define statscanf "%md %ms %*c %md %*d %*d %*d %*d %*u %*lu %*lu %*lu %*lu %*lu %*lu %*ld %*ld %*ld %*ld %mld %*ld %*llu %*lu %mld"

enum _flags { SUMMARY = 1, PPID = 2 };

typedef struct _proc {
    int pid;                //   (1) %d
    char *comm;             //   (2) %s
    int ppid;               //   (4) %d
    long int num_threads;   //  (20) %ld
    long int rss;           //  (24) %ld
} PROC;

void help();

int * getpidlist(int ppid);

int isdir(const char *dirname);

PROC ** proclist_realloc(PROC **proclist, int new_size);

int append_ppid(PROC ** proclist, const char statfilename);
#endif
