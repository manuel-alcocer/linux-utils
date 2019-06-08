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
    int opt, *pidlist;
    unsigned int flags, ppid;
    PROCLIST *proclist;

    if (argc < 2){
        help();
        exit(0);
    }

    while ((opt = getopt(argc, argv, "hsp:")) != -1){
        switch (opt){
            case 's':
                flags |= SUMMARY;
                break;
            case 'p':
                flags |= PPID;
                ppid = atoi(optarg);
                break;
            case 'h':
            default:
                help();
                exit(EXIT_FAILURE);
        }
    }

    proclist = getpidlist(ppid);

    if (proclist->pnum > 0){
        if (flags & SUMMARY == SUMMARY)
            print_table(proclist);
    }

    return 0;
}

PROCLIST * getpidlist(int ppid){
    DIR *dp;
    struct dirent *dirp;
    int dirnamepid;
    char *piddir;
    PROCLIST * local_proclist = (PROCLIST *) malloc(sizeof(PROCLIST));

    local_proclist->pnum = 0;
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
    char *pidstatfile = (char *) malloc((NAME_MAX + PATH_MAX) * sizeof(char));
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
    ssize_t nread;
    size_t len = 0;
    char *line = NULL;
    int c;

    // if error open stat file of process return -1
    if ((fp = fopen(pidstatfile, "r")) == NULL)
        return -1;

    // add 1 element for storing pid status
    proclist->pnum++;
    proclist->processes = processes_realloc(proclist);
    proclist->processes[proclist->pnum - 1] = (PROC *) malloc(sizeof(PROC));

    // c = fscanf(fp, statscanf, &f, &e, &g, &h, &i);
    c = fscanf(fp, statscanf,
            &proclist->processes[proclist->pnum - 1]->pid,
            &proclist->processes[proclist->pnum - 1]->comm,
            &proclist->processes[proclist->pnum - 1]->ppid,
            &proclist->processes[proclist->pnum - 1]->num_threads,
            &proclist->processes[proclist->pnum - 1]->rss);

    fclose(fp);

    if (c == 5 && proclist->processes[proclist->pnum - 1]->pid == pid){
        return 1;
    }

    free(proclist->processes[proclist->pnum - 1]);
    proclist->pnum--;

    return 0;
}

int scan_for_pid(const char *pidstatfile, int ppid){
    FILE *fp;
    int c, local_pid, local_ppid;

    if ((fp = fopen(pidstatfile, "r")) == NULL)
        return -1;          // error opening stat file

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

int print_table(PROCLIST *proclist){
    int l, f[] = { 3, 4, 4, 7, 3 };         // init vals: min field size
    char *linefmt, *titlefmt, *title[] = { "PID", "PPID", "THREADS", "RSS", "COMM"};

    fields_size(proclist, f);
    l = line_size(f);

    linefmt = (char *) malloc((l + 2) * sizeof(char));
    titlefmt = (char *) malloc((l + 2) * sizeof(char));

    sprintf(linefmt, " | %%%dd | %%%dd | %%%dld | %%%dld | %%-%ds |\n", f[0], f[2], f[3], f[4], f[1]);
    sprintf(titlefmt, " | %%-%ds | %%-%ds | %%-%ds | %%-%ds | %%-%ds |\n", f[0], f[2], f[3], f[4], f[1]);

    print_dashline(l);
    printf(titlefmt, title[0], title[1], title[2], title[3], title[4]);
    print_dashline(l);

    for (int i = 0; i < proclist->pnum; i++){
        printf(linefmt,
                proclist->processes[i]->pid,
                proclist->processes[i]->ppid,
                proclist->processes[i]->num_threads,
                proclist->processes[i]->rss,
                proclist->processes[i]->comm);
    }

    print_dashline(l);

}

int line_size(int * fields){
    int sum = 0;
    for (int i = 0; i < 5; i++)
        sum += fields[i] + 2 + 1;

    return (sum + 1);
}

int fields_size(PROCLIST *proclist, int *f){
    int fieldsize;
    char buff[1024];

    for (int i = 0; i < proclist->pnum; i++){
        fieldsize = sprintf(buff, "%d", proclist->processes[i]->pid);
        *f = (fieldsize > *f) ? fieldsize : *f;
        fieldsize = sprintf(buff, "%s", proclist->processes[i]->comm);
        *(f + 1) = (fieldsize > *(f + 1)) ? fieldsize : *(f + 1);
        fieldsize = sprintf(buff, "%d", proclist->processes[i]->ppid);
        *(f + 2) = (fieldsize > *(f + 2)) ? fieldsize : *(f + 2);
        fieldsize = sprintf(buff, "%ld", proclist->processes[i]->num_threads);
        *(f + 3) = (fieldsize > *(f + 3)) ? fieldsize : *(f + 3);
        fieldsize = sprintf(buff, "%ld", proclist->processes[i]->rss);
        *(f + 4) = (fieldsize > *(f + 4)) ? fieldsize : *(f + 4);
    }
}

void help(){
    printf("Ayuda\n"
            "=====\n"
            "    -p PPID    PID del proceso padre\n"
            "    -s         Muestra solo el resumen\n");
}

