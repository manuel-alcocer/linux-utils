#include "mempid.h"

#include <stdio.h>
#include <getopt.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>

int main(int argc, char **argv){
    int opt, *pidlist;
    unsigned int flags, ppid;
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

    getpidlist(ppid);

    return 0;
}

PROCLIST * getpidlist(int ppid){
    DIR *dp;
    struct dirent *dirp;
    int dirnamepid, pid;
    char *piddir, *pidstatfile;
    PROCLIST * local_proclist = (PROCLIST *) malloc(sizeof(PROCLIST));

    local_proclist->pnum = 0;
    local_proclist->processes = NULL;
    local_proclist->pagesize = sysconf(_SC_PAGESIZE);

    if ((dp = opendir(PROCDIR)) == NULL)
        exit(EXIT_FAILURE);

    while ((dirp = readdir(dp)) != NULL){
        if (sscanf(dirp->d_name, "%md%ms", &dirnamepid, &piddir) == 1){
            piddir = strdup(PROCDIR);
            // piddir = /proc + / + dirname
            sprintf(piddir, "%s/%s", piddir, dirp->d_name);
            if (isdir(piddir)){
                // piddir = /proc/pid + / + stat
                pidstatfile = strdup(piddir);
                sprintf(pidstatfile, "%s/%s", pidstatfile, STATFILENAME);
                // read ppid (main process) stat file
                if (ppid == atoi(dirp->d_name)){
                    pid = ppid;
                    append_pid(local_proclist, pidstatfile, pid);
                }
                else {
                    pid = scan_for_pid(pidstatfile, ppid);
                    if (pid >= 0)
                        append_pid(local_proclist, pidstatfile, pid);
                }
            }
        }
    }

    print_table(local_proclist);

    return local_proclist;
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
    ssize_t nread;
    size_t len = 0;
    char *line;
    int c;
    int local_pid, local_ppid;

    if ((fp = fopen(pidstatfile, "r")) == NULL)
        return -1;          // error opening stat file

    c = fscanf(fp, "%md %*s %*c %md", &local_pid, &local_ppid);

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

int print_processes(PROCLIST *proclist){
    for (int i = 0; i < proclist->pnum; i++){
        printf("%d %s %d %ld %ld\n",
                proclist->processes[i]->pid,
                proclist->processes[i]->comm,
                proclist->processes[i]->ppid,
                proclist->processes[i]->num_threads,
                proclist->processes[i]->rss);
    }
}

int print_table(PROCLIST *proclist){
    int f[] = { 0, 0, 0, 0, 0 };
    char linefmt[1024];

//    l = line_size(&f);

    printf(" | %d | %d | %d | %d | %d |", f[0], f[1], f[2], f[3], f[4]);
    //printf(" ");
    //for (int i = 0; i < l; i++)
    //    printf("-");
    //printf("\n");

    //printf(" | %%.%dd | %%.%dd | %%.%dld | %%.%dld | %%.%ds |",
    //        *f, *f + 2, *f + 3, *f + 4, *f + 1);


}

int line_size(int * fields){
    int sum = 0;
    for (int i = 0; i < 5; i++)
        sum += fields[i] + 2 + 1;

    return (sum + 1);
}

int fields_size(PROCLIST *proclist, int * *f){
    int fieldsize;
    char *buff;

    for (int i = 0; i < proclist->pnum; i++){
        fieldsize = sprintf(buff, "%d", proclist->processes[i]->pid);
        *(f[0]) = (fieldsize > *(f[0])) ? fieldsize : *(f[0]);
        printf("%d - %d\n", fieldsize, *(f[0]));
        fieldsize = sprintf(buff, "%s", proclist->processes[i]->comm);
        *(f[1]) = (fieldsize > *(f[1])) ? fieldsize : *(f[1]);
        printf("%d - %d\n", fieldsize, *(f[1]));
        fieldsize = sprintf(buff, "%d", proclist->processes[i]->ppid);
        *(f[2]) = (fieldsize > *(f[2])) ? fieldsize : *(f[2]);
        printf("%d - %d\n", fieldsize, *(f[2]));
        fieldsize = sprintf(buff, "%ld", proclist->processes[i]->num_threads);
        *(f[3]) = (fieldsize > *(f[3])) ? fieldsize : *(f[3]);
        printf("%d - %d\n", fieldsize, *(f[3]));
        fieldsize = sprintf(buff, "%ld", proclist->processes[i]->rss);
        *(f[4]) = (fieldsize > *(f[4])) ? fieldsize : *(f[4]);
        printf("%d - %d\n", fieldsize, *(f[4]));
    }
}

void help(){
    printf("Ayuda\n"
            "=====\n"
            "    -p PPID    PID del proceso padre\n"
            "    -s         Muestra solo el resumen\n");
}

