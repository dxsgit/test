#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define BUFF_SIZE 1024
#define NAME_SIZE 128
int main(int argc, char **argv)
{
    char name[NAME_SIZE];
    bzero(name,sizeof(name));
    char *p = argv[0];
    while(*p != '/'  && *p != '\0'){
        p++;    
    }
    if(*p == '\0'){
        strcpy(name, p);    
    }
    else{
        strcpy(name, p+1);    
    }
    if(argc != 2){
        printf("./%s path\n", name);
        return 0;    
    }

    int in ,out;
    in = open(argv[1], O_RDONLY);
    if(in == -1){
        printf("open %s failed!\n",argv[1]);
        return 0;    
    }
    int i = 0;
    bzero(name,sizeof(name));
    while(argv[1][i] != '\0' && argv[1][i] != '.'){
        name[i] = argv[1][i];
        i++;    
    }
    name[i] = '.';
    name[i+1] = 'o';
    name[i+2] = 'u';
    name[i+3] = 't';
    name[i+4] = '\0';

    char buf[BUFF_SIZE];
    bzero(buf,sizeof(buf));
    out = open(name, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
    int nread=0;
    while((nread = read(in,buf,sizeof(buf))) > 0){
        write(out,buf,nread);    
    }

    close(out);
    close(in);
    return 0;
    


}
