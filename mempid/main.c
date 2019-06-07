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

int * getpidlist(int ppid){
    DIR *dp;
    struct dirent *dirp;
    int dirnamepid;
    char *piddir, *pidstatfile;
    int *local_pidlist = (int *) malloc(4096*sizeof(int));
    PROC * *proclist = NULL;

    if ((dp = opendir(PROCDIR)) == NULL)
        exit(EXIT_FAILURE);

    while ((dirp = readdir(dp)) != NULL){
        if (sscanf(dirp->d_name, "%md%ms", &dirnamepid, &piddir) == 1){
            piddir = strdup(PROCDIR);
            // piddir = /proc + / + dirname
            sprintf(piddir, "%s/%s", piddir, dirp->d_name);
            if (isdir(piddir)){
                // piddir = /proc/pid + / + stat
                sprintf(pidstatfile, "%s/%s", piddir, STATFILENAME);
                // read stat file
                if (ppid == atoi(dirp->d_name))
                    append_ppid(local_pidlist, pidstatfile);
                else
                    scan_for_ppid(local_pidlist, pidstatfile);
            }
        }
    }

    return local_pidlist;
}

int append_ppid(PROC ** proclist, const char statfilename){

}

int isdir(const char *dirname){
    struct stat dir;

    if (stat(dirname, &dir) == 0 && S_ISDIR(dir.st_mode))
        return 1;
    return 0;
}

PROC ** proclist_realloc(PROC **proclist, int new_size){
    if (proclist == NULL || new_size == 1)
        return (PROC **) malloc(sizeof(PROC *));
    else if (new_size > 1)
        return (PROC **) realloc(proclist, new_size * sizeof(PROC *));
    else
        return NULL;
}

void help(){
    printf("Ayuda\n"
            "=====\n"
            "    -p PPID    PID del proceso padre\n"
            "    -s         Muestra solo el resumen\n");
}
