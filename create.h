#include <stdint.h>
#define NAME 100
#define MODE 8
#define UID 8
#define GID 8
#define MODE 8
#define SIZE 12
#define MTIME 12
#define CHKSUM 8
#define LINKNAME 100
#define MAGIC 6
#define VERSION 2
#define UNAME 32
#define GNAME 32
#define DEVMAJOR 8
#define DEVMINOR 8
#define PREFIX 155
#define BLOCKSIZE 512
typedef struct headers{
    char name[NAME];
    char mode[MODE];
    char uid[UID];
    char gid[GID];
    char size[SIZE];
    char mtime[MTIME];
    char chksum[CHKSUM];
    char typeflag;
    char linkname[LINKNAME];
    char magic[MAGIC];
    char version[VERSION];
    char uname[UNAME];
    char gname[GNAME];
    char devmajor[DEVMAJOR];
    char devminor[DEVMINOR];
char prefix[PREFIX];
} Header;
void pophead(Header *header, char *fname, char *cwd);
void preorder(char *path, int outfd, int verbose);
void write_head(Header *header, int outfd);
uint8_t calc_checksum(Header *header);
void name_helper(Header *header, char *name);
