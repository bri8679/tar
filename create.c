#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <pwd.h>
#include <sys/stat.h>
#include <limits.h>
#include <grp.h>
#include <stdint.h>
#include "create.h"
#define EMPTYBYTES 12
void pophead(Header *header, char *fname, char *cwd){
    /*populate the header structure*/
    struct stat sb;
    struct passwd *pwd;
    struct group *grp;
    struct dirent;
    char *name = malloc(strlen(cwd) + 1);
    uint8_t checksum;
    strcpy(name, cwd);

    /*initalize fields*/
    memset(header->name, '\0', sizeof(header->name));
    memset(header->chksum, ' ', sizeof(header->chksum));
    memset(header->linkname, '\0', sizeof(header->linkname));
    memset(header->gname, '\0', sizeof(header->gname));
    memset(header->uname, '\0', sizeof(header->uname));
    memset(header->devmajor, '\0', sizeof(header->devmajor));
    memset(header->devminor, '\0', sizeof(header->devminor));
    memset(header->prefix, '\0', sizeof(header->prefix));

    if(lstat(fname, &sb) == -1){
        perror("Could not stat");
        exit(EXIT_FAILURE);
    }
    strcat(name, "/");
    if(S_ISDIR(sb.st_mode)){
        header->typeflag = '5';
        memset(header->size, '\0', SIZE);
    }
    else if(S_ISLNK(sb.st_mode)){
        header->typeflag = '2';
        snprintf(header->size, SIZE, "%011o", (unsigned int)sb.st_size);
        strcat(name, fname);
        readlink(name, header->linkname, LINKNAME);
    }
    else if(S_ISREG(sb.st_mode)){
        header->typeflag = '0';
        snprintf(header->size, SIZE, "%011o", (unsigned int)sb.st_size);
        strcat(name, fname);
    }
    if(strlen(name) > NAME){
            name_helper(header, name);
    }
    else{
        strncpy(header->name, name, strlen(name));
    }
    snprintf(header->mode, MODE, "%07o", sb.st_mode);
    snprintf(header->uid, UID, "%07o", sb.st_uid);
    snprintf(header->gid, GID, "%07o", sb.st_gid);
    snprintf(header->mtime, MTIME, "%011o", (unsigned int)sb.st_mtime);
    strcpy(header->magic, "ustar");
    strcpy(header->version, "00");
 if((pwd = getpwuid(sb.st_uid)) == NULL){
        perror("Couldn't get pwd");
        exit(EXIT_FAILURE);
    }
    if(strlen(pwd->pw_name) >= UNAME){
        /*if the name is to long*/
        strncpy(header->uname, pwd->pw_name, UNAME-1);
        header->uname[UNAME-1] = '\0';
    }
    else{
        strncpy(header->uname, pwd->pw_name, strlen(pwd->pw_name));
    }
    if((grp = getgrgid(sb.st_gid)) == NULL){
        perror("Couldn't get pwd");
        exit(EXIT_FAILURE);
    }
    if(strlen(grp->gr_name) >= GNAME){
        strncpy(header->gname, grp->gr_name, GNAME-1);
        header->gname[GNAME-1] = '\0';
    }
    else{
        strncpy(header->gname, grp->gr_name, strlen(grp->gr_name));
    }
    checksum = calc_checksum(header);
    snprintf(header->chksum, CHKSUM, "%07o", checksum);
    return;
}

void preorder(char *path, int outfd, int verbose){
    /*Does a preorder traversal through directory tree,
    makes a header for each file/directory and writes out the
    header and contents of it is a file*/
    DIR *dir;
    Header *header;
 struct dirent *entry;
    struct stat sb;
    char *buff = malloc(BLOCKSIZE);
    int infd, write_out, space_left;
    int total_write = 0;
    char *cwd = malloc(PATH_MAX);
    char *wrtnull;

    if((dir = opendir(path)) == NULL){
        perror("Couldn't open the directory");
        exit(EXIT_FAILURE);
    }
    chdir(path);
    while((entry = readdir(dir))){
        if(lstat(entry->d_name, &sb) == -1){
            perror("Stat failed");
            exit(EXIT_FAILURE);
        }
        if(getcwd(cwd, sizeof(PATH_MAX)) == NULL){
                perror("Couldn't get cwd");
                exit(EXIT_FAILURE);
        }
        if(S_ISDIR(sb.st_mode)){
            if(!strcmp(".", entry->d_name) || !strcmp("..", entry->d_name)){
                continue;
            }
            header = malloc(sizeof(Header));
            if(verbose){
                printf("%s\n", cwd);
            }
            pophead(header, entry->d_name, cwd);
            write_head(header, outfd);
            free(header);
            preorder(entry->d_name, outfd, verbose);
}
        else{
            /*populate the header, write it out, and write out the contents*/
            if((infd = open(entry->d_name, O_RDONLY)) == -1){
                perror("Can't open file");
                exit(EXIT_FAILURE);
            }
            header = malloc(sizeof(Header));
            if(verbose){
                printf("%s/", cwd);
                printf("%s\n", entry->d_name);
            }
            pophead(header, entry->d_name, cwd);
            write_head(header, outfd);
            free(header);
            while((write_out = read(infd, buff, BLOCKSIZE))){
                write(outfd, buff, write_out);
                total_write += write_out;
            }
            if((space_left = total_write % BLOCKSIZE) != 0){
                wrtnull = malloc(space_left*sizeof(char));
                memset(wrtnull, '\0', space_left);
                write(outfd, wrtnull, space_left);
                free(wrtnull);
            }
            close(infd);
        }
    }
    chdir("..");
    free(cwd);
    closedir(dir);
}

void write_head(Header *header, int outfd){
/*Write out the header and its extra 12 bytes*/
    char *empbytes = malloc(EMPTYBYTES*sizeof(char));
    memset(empbytes, '\0', EMPTYBYTES);

    write(outfd, header->name, sizeof(header->name));
    write(outfd, header->mode, sizeof(header->mode));
    write(outfd, header->uid, sizeof(header->uid));
    write(outfd, header->gid, sizeof(header->gid));
    write(outfd, header->size, sizeof(header->size));
    write(outfd, header->mtime, sizeof(header->mtime));
    write(outfd, header->chksum, sizeof(header->chksum));
    write(outfd, &header->typeflag, sizeof(header->typeflag));
    write(outfd, header->linkname, sizeof(header->linkname));
    write(outfd, header->magic, sizeof(header->magic));
    write(outfd, header->version, sizeof(header->version));
    write(outfd, header->uname, sizeof(header->uname));
    write(outfd, header->gname, sizeof(header->gname));
    write(outfd, header->devmajor, sizeof(header->devmajor));
    write(outfd, header->devminor, sizeof(header->devminor));
    write(outfd, header->prefix, sizeof(header->prefix));
    write(outfd, empbytes, EMPTYBYTES);
}
uint8_t calc_checksum(Header *header){
    char *temp = header->name;
    uint8_t sum = 0;
    int i;
    for(i = 0; i < BLOCKSIZE; i++){
        if(*temp){
            sum++;
        }
        temp++;
    }
    return sum;
}
void name_helper(Header *header, char *name){
    /*finds the cutoff for name
    and puts the rest into the prefix*/
    int i;
    char *cutoff;
    char *spot = name;
    int prelen;
    for(i = NAME; i > 0; i--){
        if(*spot == '/'){
            cutoff = spot++;
        }
        spot--;
    }
    memmove(header->name, cutoff, strlen(cutoff));
    if((prelen = strlen(name) - NAME) > PREFIX){
        fprintf(stderr, "Path is to long");
        exit(EXIT_FAILURE);
    }
    memmove(header->prefix, name, prelen);
    if(prelen < PREFIX){
    /*add in the null if it fits*/
        header->prefix[prelen] = '\0';
    }
}
