#ifndef _MEMPID_H_
#define _MEMPID_H_

#define PROCDIR "/proc"
#define STATFILENAME "stat"

#define statscanf "%d (%m[^)]) %*c %d %*d %*d %*d %*d %*u %*u %*u %*u %*u %*u %*u %*d %*d %*d %*d %ld %*d %*u %*u %ld"

enum _flags { SUMMARY = 1, PPID = 2 };

typedef struct _proc {
    int pid;                //   (1) %d
    char *comm;             //   (2) %s
    int ppid;               //   (4) %d
    long int num_threads;   //  (20) %ld
    long int rss;           //  (24) %ld
    long int mem;           //   rss * pagesize
} PROC;

typedef struct _proclist {
    int pnum;
    long int pagesize;
    PROC ** processes;
} PROCLIST;

void help();

PROCLIST * getpidlist(int ppid);

int isdir(const char *dirname);

PROC ** processes_realloc(PROCLIST * proclist);

int append_pid(PROCLIST * proclist, const char *pidstatfile, int pid);

int scan_for_pid(const char *pidstatfile, int ppid);

int print_table(PROCLIST *proclist);

int fields_size(PROCLIST *proclist, int f[]);

int line_size(int * fields);

int read_statfile(PROCLIST * proclist, const char *dirname, int ppid);
#endif
