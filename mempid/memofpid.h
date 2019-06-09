#ifndef _MEMPID_H_
#define _MEMPID_H_

#define PROCDIR "/proc"
#define STATFILENAME "stat"

#define statscanf "%d (%m[^)]) %*c %d %*d %*d %*d %*d %*u %*u %*u %*u %*u %*u %*u %*d %*d %*d %*d %ld %*d %*u %*u %ld"

enum _flags { TSUMMARY = 1, PPID = 2 };

enum _units_pow { Byte = 'B', KiB = 'K', MiB = 'M', GiB = 'G' };

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
    unsigned long int total_mem;
    char *unitsstr;
    int factor;
} PROCLIST;

void help();

PROCLIST * getpidlist(int ppid);

int isdir(const char *dirname);

PROC ** processes_realloc(PROCLIST * proclist);

int append_pid(PROCLIST * proclist, const char *pidstatfile, int pid);

int scan_for_pid(const char *pidstatfile, int ppid);

void print_table(PROCLIST *proclist);

void print_total_mem(PROCLIST *proclist);

void fields_size(PROCLIST *proclist, int *f);

int line_size(int * fields);

int read_statfile(PROCLIST * proclist, const char *dirname, int ppid);

int bpow(int num, int pow);

int fill_units(PROCLIST *proclist, const char *unitsstr);

#endif
