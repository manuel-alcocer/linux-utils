#include "memofpid.h"

#include <stdio.h>
#include <getopt.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <limits.h>

int main(int argc, char **argv){
    int opt;
    long ppid = -1;
    char *endptr = NULL, *unitstr = "B";
    unsigned int flags = 0;
    PROCLIST *proclist;

    if (argc < 2){
        help();
        exit(0);
    }

    while ((opt = getopt(argc, argv, "htp:u:")) != -1){
        switch (opt){
            case 't':
                flags |= TSUMMARY;
                break;
            case 'p':
                ppid = strtol(optarg, &endptr, 10);
                flags |= (endptr == optarg) ? 0 : PPID;
                break;
            case 'u':
                unitstr = strdup(optarg);
                break;
            case 'h':
            default:
                help();
                exit(EXIT_FAILURE);
        }
    }

    if ((flags & PPID) != PPID){
        help();
        exit(1);
    }

    proclist = getpidlist(ppid);

    fill_units(proclist, unitstr);

    if (proclist->pnum > 0){
        if ((flags & TSUMMARY) == TSUMMARY)
            print_table(proclist);
        else
            print_total_mem(proclist);
    }

    return 0;
}

// I know.. casting from long to int!!! A PID can be a long??
PROCLIST * getpidlist(int ppid){
    DIR *dp;
    struct dirent *dirp;
    int dirnamepid;
    char *piddir;
    PROCLIST * local_proclist = (PROCLIST *) malloc(sizeof(PROCLIST));

    local_proclist->pnum = 0;
    local_proclist->total_mem = 0;
    local_proclist->processes = NULL;
    local_proclist->pagesize = sysconf(_SC_PAGESIZE);

    if ((dp = opendir(PROCDIR)) == NULL)
        exit(EXIT_FAILURE);

    while ((dirp = readdir(dp)) != NULL){
        if (sscanf(dirp->d_name, "%d%ms", &dirnamepid, &piddir) == 1){
            piddir = strdup(PROCDIR);
            sprintf(piddir, "%s/%s", piddir, dirp->d_name);
            if (isdir(piddir))
                read_statfile(local_proclist, dirp->d_name, ppid);
        }
    }

    return local_proclist;
}

int read_statfile(PROCLIST * proclist, const char *dirname, int ppid){
    char *pidstatfile = (char *) malloc(FILENAME_MAX  * sizeof(char));
    int pid;

    sprintf(pidstatfile, "%s/%s/%s", PROCDIR, dirname, STATFILENAME);
    if (ppid == atoi(dirname)){
        pid = ppid;
        append_pid(proclist, pidstatfile, pid);
    }
    else {
        pid = scan_for_pid(pidstatfile, ppid);
        if (pid >= 0)
            append_pid(proclist, pidstatfile, pid);
    }
}

int append_pid(PROCLIST * proclist, const char *pidstatfile, int pid){
    FILE *fp;
    int c;

    if ((fp = fopen(pidstatfile, "r")) == NULL)
        return -1;

    proclist->pnum++;
    proclist->processes = processes_realloc(proclist);
    proclist->processes[proclist->pnum - 1] = (PROC *) malloc(sizeof(PROC));

    c = fscanf(fp, statscanf,
            &proclist->processes[proclist->pnum - 1]->pid,
            &proclist->processes[proclist->pnum - 1]->comm,
            &proclist->processes[proclist->pnum - 1]->ppid,
            &proclist->processes[proclist->pnum - 1]->num_threads,
            &proclist->processes[proclist->pnum - 1]->rss);
    fclose(fp);

    if (c == 5 && proclist->processes[proclist->pnum - 1]->pid == pid){
        proclist->processes[proclist->pnum - 1]->mem =
            proclist->pagesize * proclist->processes[proclist->pnum - 1]->rss;
        proclist->total_mem += proclist->processes[proclist->pnum -1]->mem;

        return 1;
    } else {
        free(proclist->processes[proclist->pnum - 1]);
        proclist->pnum--;
    }

    return 0;
}

int scan_for_pid(const char *pidstatfile, int ppid){
    FILE *fp;
    int c, local_pid, local_ppid;

    if ((fp = fopen(pidstatfile, "r")) == NULL)
        return -1;

    c = fscanf(fp, "%d %*s %*c %d", &local_pid, &local_ppid);

    if (c == 2 && local_ppid == ppid)
        return local_pid;

    return -1;
}

int isdir(const char *dirname){
    struct stat dir;

    if (stat(dirname, &dir) == 0 && S_ISDIR(dir.st_mode))
        return 1;
    return 0;
}

PROC ** processes_realloc(PROCLIST * proclist){
    if (proclist->processes == NULL)
        return (PROC **) malloc(sizeof(PROC *));
    else if (proclist->pnum > 0)
        return (PROC **) realloc(proclist->processes, proclist->pnum * sizeof(PROC *));
    else
        return NULL;
}

void print_dashline(int length){
    printf(" ");
    for (int i = 0; i < length; i++)
        printf("-");
    printf("\n");
}

void print_total_mem(PROCLIST *proclist){
    printf("%lu\n", proclist->total_mem / proclist->factor);
}

void print_table(PROCLIST *proclist){
    // VERY DIRTY FUNC!!! NEED CLEANING!!
    char *total = "Total memory";
    int l, f[] = { 3, 4, 4, 7, 3, 0 };         // init vals: min field size
    char memcol[64];
    char *linefmt, *titlefmt, *totalfmt, *title[] = { "PID", "PPID", "THREADS", "RSS","COMM", "MEM" };

    f[5] = sprintf(memcol, "%s [%s]", title[5], proclist->unitsstr);
    fields_size(proclist, f);
    l = line_size(f);

    linefmt = (char *) malloc((l + 2) * sizeof(char));
    titlefmt = (char *) malloc((l + 2) * sizeof(char));
    totalfmt = (char *) malloc((l + 2) * sizeof(char));

    sprintf(linefmt, " | %%%dd | %%%dd | %%%dld | %%%dld | %%-%ds | %%%dld |\n", f[0], f[2], f[3], f[4], f[1], f[5]);
    sprintf(titlefmt, " | %%-%ds | %%-%ds | %%-%ds | %%-%ds | %%-%ds | %%-%ds |\n", f[0], f[2], f[3], f[4], f[1], f[5]);
    sprintf(totalfmt, " | %%%ds | %%%dlu |\n", l - 7 - f[5], f[5]);

    print_dashline(l);
    printf(titlefmt, title[0], title[1], title[2], title[3], title[4], memcol);
    print_dashline(l);

    for (int i = 0; i < proclist->pnum; i++){
        printf(linefmt,
                proclist->processes[i]->pid,
                proclist->processes[i]->ppid,
                proclist->processes[i]->num_threads,
                proclist->processes[i]->rss,
                proclist->processes[i]->comm,
                proclist->processes[i]->mem / proclist->factor);
    }

    print_dashline(l);
    printf(totalfmt, total, proclist->total_mem / proclist->factor);
    print_dashline(l);
}

int line_size(int * fields){
    int sum = 0;
    for (int i = 0; i < 6; i++)
        sum += fields[i] + 2 + 1;

    return (sum + 1);
}

void fields_size(PROCLIST *proclist, int *f){
    int c;
    char buff[64];

    for (int i = 0; i < proclist->pnum; i++){
        *f = ((c = intsize(buff,proclist->processes[i]->pid)) > *f) ? c : *f;
        *(f + 1) = ((c = strsize(buff, proclist->processes[i]->comm)) > *(f + 1)) ? c : *(f + 1);
        *(f + 2) = ((c = intsize(buff, proclist->processes[i]->ppid)) > *(f + 2)) ? c : *(f + 2);
        *(f + 3) = ((c = lintsize(buff, proclist->processes[i]->num_threads)) > *(f + 3)) ? c : *(f + 3);
        *(f + 4) = ((c = lintsize(buff, proclist->processes[i]->rss)) > *(f + 4)) ? c : *(f + 4);
        *(f + 5) = ((c = lintsize(buff, proclist->processes[i]->mem)) > *(f + 5)) ? c : *(f + 5);
    }
}

int fill_units(PROCLIST *proclist, const char *unitsstr){
    if (!strcmp("B", unitsstr)){
        proclist->unitsstr = "Bytes";
        proclist->factor = bpow(1024, 0);
    }
    else if (!strcmp("K", unitsstr)){
        proclist->unitsstr = "KiB";
        proclist->factor = bpow(1024, 1);
    }
    else if (!strcmp("M", unitsstr)){
        proclist->unitsstr = "MiB";
        proclist->factor = bpow(1024, 2);
    }
    else if (!strcmp("G", unitsstr)){
        proclist->unitsstr = "GiB";
        proclist->factor = bpow(1024, 3);
    } else {
        proclist->unitsstr = "Bytes";
        proclist->factor = bpow(1024, 0);
    }

    return proclist->factor;
}

int bpow(int num, int pow){
    int result = 1;

    for (int i = 0; i < pow; i++)
        result *= num;

    return result;
}

void help(){
    printf("Ayuda\n"
            "=====\n"
            "    -p PPID    PID del proceso padre\n"
            "    -t         Muestra una tabla resumen\n"
            "    -u <unit>  Unidad de representación\n"
            "                 Valores: B, K, M, G\n");
}

