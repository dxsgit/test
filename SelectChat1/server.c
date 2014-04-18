#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#define MAX_SIZE 1024

#define CLIENT_MAX 10

typedef struct cList{
    int fd;
    struct cList * next;
}CList;

void initCList(CList *cl)
{
    cl = NULL;
}

CList* addClient(CList **pcl, int fd)
{
    CList *cl = *pcl;
    CList *p = (CList*)malloc(sizeof(CList));
    p->fd = fd;
    p->next = NULL;
    if(cl == NULL){
        cl = p;
    }
    else{
        CList *q = cl;
        while(q->next != NULL && q->fd > fd){
            q = q->next;
        }
        if(q == cl){
            cl = p;
            p->next = q;
        }else{
            q->next = p;
        }
    }
    *pcl = cl;
    return cl;
}

CList* delClientNext(CList **pcl, CList *pl)
{
    CList *cl = *pcl;
    if(cl == NULL || pl == NULL){
        return NULL;
    }
    else if(cl == pl){
        cl = cl->next;
        *pcl = cl;
        printf("关闭客户端:%d\n",pl->fd);
        close(pl->fd);
        free(pl);
    }
    else{
        while(cl->next != NULL && cl->next != pl){
            cl = cl->next;
        }
        if(cl != NULL){
            cl = pl->next;
            printf("关闭客户端:%d\n",pl->fd);
            close(pl->fd);
            free(pl);
        }
    }
    *pcl = cl;
    return cl;
}
CList* delClient(CList **pcl, int fd)
{
    CList *cl = *pcl;
    if(cl == NULL){
        return NULL;
    }
    else if(cl->fd == fd){
        CList *p = cl;
        cl = cl->next;
        printf("关闭客户端:%d\n",p->fd);
        close(p->fd);
        free(p);
    }
    else{
        CList *p = cl;
        while(p->next != NULL && p->next->fd != fd){
            p = p->next;
        }
        if(p != NULL){
            CList *q = p->next;
            p = q->next;
            printf("关闭客户端:%d\n",q->fd);
            close(q->fd);
            free(q);
        }
    }
    *pcl = cl;
    return cl;
}
void destoryCList(CList **pcl){
    printf("destory list\n");
    CList *p = *pcl;
    while(p != NULL){
        CList *q = p;
        p = p->next;
        printf("关闭客户端:%d\n",q->fd);
        close(q->fd);
        free(q);
    }
   *pcl = NULL;
}

//设置两个全局变量
CList *g_clientList = NULL;
int g_fd = -1;

//处理Ctrl + C中断
void sigfun(int sig)
{
    destoryCList(&g_clientList);
    if(g_fd > 0){
        printf("关闭服务器:%d\n",g_fd);
        close(g_fd);
    }
}


void addUserFD(CList *cl,fd_set *rfds)
{
    while(cl != NULL){
        FD_SET(cl->fd,rfds);
        cl = cl->next;
    }
}

#define MAX_NUM 5
int main(int argc, char **argv)
{
    int c_fd;
    socklen_t len;
    struct sockaddr_in s_addr, c_addr;
    unsigned short port = 6000, num = 5;
    char ip[16];
    strcpy(ip,"127.0.0.1");
    
    fd_set  rfds;
    int max_fd= 0;
    char buf[MAX_SIZE+1];
    int opt;
    while((opt = getopt(argc, argv, ":p:i:n:")) != -1){
        switch(opt){
            case 'p':
                port = atoi(optarg);
                break;
            case 'n':
                num = atoi(optarg);
                break;
            case 'i':
                strcpy(ip, optarg);
                break;
            default:
                printf("%s [-p port] [-i ipaddr] [-n number]\n", argv[0]);
                return 1;
        }
    }
   
    // 设置信号量
    signal(SIGINT, sigfun);

    // 创建socket
    if((g_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        perror("socket create failed!");
        return 1;
    }

    // 设置地址
    bzero(&s_addr, sizeof(s_addr));
    s_addr.sin_family = AF_INET;
    s_addr.sin_addr.s_addr = inet_addr(ip);
    s_addr.sin_port = htons(port);
    
    // 绑定socket
    if(bind(g_fd, (struct sockaddr *)&s_addr, sizeof(struct sockaddr)) == -1){
        perror("socket bind failed!");
        return 1;
    }

    // 监听socket
    if(listen(g_fd, num) == -1){
        perror("socket listen failed!");
        return 1;
    }

    int cur_fd = -1;
    int i = 0;
    printf("------等待客户端建立连接请求-------\n");

    // 处理收发数据
    while(1){
        struct timeval tv = {1,0};
        FD_ZERO(&rfds);
        FD_SET(0, &rfds);
        FD_SET(g_fd, &rfds);
        max_fd = g_fd;
        if(g_clientList != NULL){
            addUserFD(g_clientList,&rfds); 
            max_fd = g_clientList->fd;
        }
        int rst = select(max_fd+1,&rfds,NULL,NULL,&tv);

        if(rst == -1){
            printf("聊天终止，select出错，信息：%s\n",strerror(errno));
            break;
        }
        else if (rst > 0){
            CList *p = NULL;
            if(FD_ISSET(0,&rfds) && cur_fd != -1){
                // 发送数据
                bzero(buf,sizeof(buf));
                //                printf("请输入发给客户端的消息：\n");
                fgets(buf, MAX_SIZE, stdin);
                if( !strncasecmp(buf, "quit", 4)){
                    printf("退出聊天\n");
                    break;
                }
                len = send(cur_fd, buf, strlen(buf) - 1, 0);
                if(len > 0){
                    printf("S -> C [%d]:%s \n", cur_fd, buf);
                }
                else{
                    printf("消息;%s 发送失败,错误信息:%s\n",buf,strerror(errno));
                    printf("聊天终止！\n");
                    break;
                }
            }
            if(FD_ISSET(g_fd,&rfds)){
                len = sizeof(struct sockaddr);
                if((c_fd = accept(g_fd, (struct sockaddr *)&c_addr, &len)) == -1){
                    perror("socket accept failed");
                    return 1;
                }
                // 打印客户端的ip和port
                printf("客户SOCK:%d,客户端IP:%s,端口号:%d\n",c_fd, inet_ntoa(c_addr.sin_addr), c_addr.sin_port);
                
                //插入数据
                addClient(&g_clientList, c_fd);
               // FD_SET(c_fd, &rfds);
                if(g_clientList != NULL && g_clientList->fd > max_fd){
                    max_fd = g_clientList->fd;
                }
            }
            // 遍历客户端发来的数据
            p = g_clientList;
            while(p != NULL ){
                if(FD_ISSET(p->fd, &rfds)){
                    printf("ok client: fd = %d\n",p->fd);
                    // 接收数据
                    c_fd = p->fd;
                    bzero(buf,sizeof(buf));
                    len = recv(c_fd, buf, MAX_SIZE, 0);
                    if(len > 0){
                        cur_fd = c_fd;
                        printf("C[%d] -> S [%d]:%s \n", i, c_fd, buf);
                        sleep(20);
                        p = p->next;
                    }
                    else if(len < 0){
                        printf("消息接收失败,错误信息:%s\n", strerror(errno));
                        p = delClientNext(&g_clientList,p);
                        printf("聊天终止！\n");
                        continue;
                    }
                    else{
                        printf("客户端退出聊天！\n");
                        p = delClientNext(&g_clientList,p);
                        printf("聊天终止！\n");
                        continue;

                    }
                }
                else{
                    p = p->next;
                }
            }
        }
    }
    destoryCList(&g_clientList);
    close(g_fd);
    return 0;
}
