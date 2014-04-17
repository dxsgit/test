#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#define MAX_SIZE 1024

int main(int argc, char **argv)
{
    int sockfd;
    socklen_t len;
    struct sockaddr_in addr;
    unsigned short port = 6000, num = 5;
    char ip[16];
    strcpy(ip,"127.0.0.1");

    fd_set  rfds;

    char buf[MAX_SIZE+1];
    int opt;
    while((opt = getopt(argc, argv, ":p:i:")) != -1){
        switch(opt){
            case 'p':
                port = atoi(optarg);
                break;
            case 'i':
                strcpy(ip, optarg);
                break;
            default:
                printf("%s [-p port] [-i ipaddr] \n", argv[0]);
                return 1;
        }
    }
    
    // 创建socket
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        perror("socket create failed!");
        return 1;
    }

    // 设置地址
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip);
    addr.sin_port = htons(port);
    
    // 连接服务器
    if(connect(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr)) == -1){
        perror("socket connect error!");
        printf("错误信息：%s\n",strerror(errno));
        return 1;
    }

    printf("建立连接成功！\n");


    // 处理收发数据
    while(1){
        FD_ZERO(&rfds);
        FD_SET(0, &rfds);
        FD_SET(sockfd, &rfds);
        struct timeval tv = {1,0};
        int rst = select(sockfd+1,&rfds,NULL,NULL,&tv);

        if(rst == -1){
            printf("聊天终止，select出错，信息：%s\n",strerror(errno));
            break;
        }
        else if (rst > 0){

            if(FD_ISSET(sockfd,&rfds)){
                // 接收数据
                bzero(buf,sizeof(buf));
                len = recv(sockfd, buf, MAX_SIZE, 0);
                if(len > 0){
                    printf("S -> C [%d]:%s \n", sockfd, buf);
                }
                else if(len < 0){
                    printf("消息接收失败,错误信息:%s\n", strerror(errno));
                    printf("聊天终止！\n");
                    break;
                }
                else{
                    printf("服务端退出聊天！\n");
                    printf("聊天终止！\n");
                    break;
                }

            }
            else if(FD_ISSET(0,&rfds)){
                // 发送数据
                bzero(buf,sizeof(buf));
//                getchar();
//                printf("请输入发给客户端的消息：\n");
                fgets(buf, MAX_SIZE, stdin);
                if( !strncasecmp(buf, "quit", 4)){
                    printf("退出聊天\n");
                    break;
                }

                len = send(sockfd, buf, strlen(buf) - 1, 0);
                if(len > 0){
                    printf("C -> S [%d]:%s \n", sockfd, buf);
                }
                else{
                    printf("消息;%s 发送失败,错误信息:%s\n",buf,strerror(errno));
                    printf("聊天终止！\n");
                    break;
                }
            }
        }
    }
    FD_ZERO(&rfds);
    close(sockfd);
    return 0;
}
