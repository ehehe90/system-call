#include <stdio.h>
#include <stdlib.h>
//#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <time.h>
#define mem_alloc(p, type, size, errno)                                        \
    do {                                                                       \
        if ((p = (type *)malloc(size)) == NULL) {                              \
            fprintf(stderr, "Could not allocate %lu memory\n", (size));        \
            exit(errno);                                                       \
        }                                                                      \
    } while (0)
#define STRING_SIZE 256
#define FILE_NUM 1024

void get_mode(mode_t mode, char **p_string)
{
    mem_alloc(*p_string, char, sizeof(char) * 11, 1);
    (*p_string)[0] = S_ISBLK(mode) ? 'b' :
                   S_ISCHR(mode) ? 'c' :
                   S_ISDIR(mode) ? 'd' :
                   S_ISLNK(mode) ? 'l' :
                   S_ISFIFO(mode) ? 'p' :
                   S_ISSOCK(mode) ? 's' :
                   S_ISREG(mode) ? '-' : '?';
    (*p_string)[1] = (mode & S_IRUSR) ? 'r' : '-';
    (*p_string)[2] = (mode & S_IWUSR) ? 'w' : '-';
    (*p_string)[3] = (mode & S_IXUSR) ? 'x' : '-';
    (*p_string)[4] = (mode & S_IRGRP) ? 'r' : '-';
    (*p_string)[5] = (mode & S_IWGRP) ? 'w' : '-';
    (*p_string)[6] = (mode & S_IXGRP) ? 'x' : '-';
    (*p_string)[7] = (mode & S_IROTH) ? 'r' : '-';
    (*p_string)[8] = (mode & S_IWOTH) ? 'w' : '-';
    (*p_string)[9] = (mode & S_IXOTH) ? 'x' : '-';
    (*p_string)[10] = '\0';
}

int print_files(char *path, char ***p_array_string)
{
    int i = 0, j = 0;
    struct stat st;
    mode_t mode;
    char *s;
    struct tm *time;
    DIR *dir;
    struct dirent *dp;
    //printf("path : %s!\n", path);
    if (stat(path, &st) < 0) {
        perror("stat");
        return -1;
    }
    if (S_ISDIR(st.st_mode)) {
        if ((dir = opendir(path)) == NULL) {
            perror("opendir");
            return -1;
        }
        mem_alloc(*p_array_string, char *, sizeof(char *) * FILE_NUM, 1);
        for (dp = readdir(dir); dp != NULL; dp = readdir(dir)) {
            mem_alloc((*p_array_string)[i], char, sizeof(char) * STRING_SIZE, 1);
            stat(dp->d_name, &st);
            mode = st.st_mode;
            get_mode(mode, &s);
            time = localtime(&st.st_atime);
            snprintf((*p_array_string)[i], STRING_SIZE, "%s %2d %8lld %d %d %02d:%02d %s", s, st.st_nlink, st.st_size ,time->tm_mon + 1, time->tm_mday, time->tm_hour, time->tm_min, dp->d_name);
            //printf("%s %2d %8lld %d %d %02d:%02d %s\n", s, st.st_nlink, st.st_size ,time->tm_mon + 1, time->tm_mday, time->tm_hour, time->tm_min, dp->d_name);
            i++;
            /*
            printf("size : 0x%x\n", i * sizeof(char *))
            if (realloc(*p_array_string, i * sizeof(char *)) == NULL) {
                fprintf(stderr, "realloc error\n");
                exit(1);
            }
            */
        }
        closedir(dir);
    } else {
        mem_alloc(*p_array_string, char *, sizeof(char *), 1);
        mode = st.st_mode;
        mem_alloc((*p_array_string)[i], char, sizeof(char) * STRING_SIZE, 1);
        get_mode(mode, &s);
        time = localtime(&st.st_atime);
        snprintf((*p_array_string)[i], STRING_SIZE, "%s %2d %8lld %d %d %02d:%02d %s", s, st.st_nlink, st.st_size ,time->tm_mon + 1, time->tm_mday, time->tm_hour, time->tm_min, path);
        i++;
    }
    return i;
}