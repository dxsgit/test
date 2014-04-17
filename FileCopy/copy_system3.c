#include <stdio.h>
#include <stdlib.h>

#define NAME_SIZE 128
#define BUF_SIZE 1024
int main(int argc, char **argv)
{

    if(argc != 2){
        printf("./%s path\n",argv[0]);
        return 0;
    }
    FILE *fp_in = NULL;
    fp_in = fopen(argv[1],"r");
    if(fp_in == NULL){
        printf("open %s failed!\n",argv[1]) ;
        return 0;   
    }

    
    char name[NAME_SIZE];
    bzero(name,sizeof(name));

    int i = 0;
    while(argv[1][i] != '.' && argv[1][i] != '\0'){
        name[i] = argv[1][i];
        i++; 
    }
    name[i] = '.';
    name[i+1] = 'b';
    name[i+2] = 'a';
    name[i+3] = 't';
    name[i+4] = '\0';

    FILE *fp_out= open(name,"w");
    if(fp_out == NULL){
        fclose(fp_in);
        printf("create %s failed!\n",name);
        return 0;    
    }
    char buf[BUF_SIZE];
    while( fgets(buf, sizeof(buf),fp_in) != NULL){
        fw    
    }
    

    return 0;

}
