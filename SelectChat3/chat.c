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

typedef struct NetData{
    char name[14];
    char buf[MAX_SIZE+1];
}NetData;

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
        printf("处决囚犯:%d\n",pl->fd);
        close(pl->fd);
        free(pl);
    }
    else{
        while(cl->next != NULL && cl->next != pl){
            cl = cl->next;
        }
        if(cl != NULL){
            cl = pl->next;
            printf("处决囚犯:%d\n",pl->fd);
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
        printf("处决囚犯:%d\n",p->fd);
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
            printf("处决囚犯:%d\n",q->fd);
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
        printf("处决囚犯:%d\n",q->fd);
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
        printf("关闭牢房:%d\n",g_fd);
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

int dealClient(int sockfd)
{
    // 处理收发数据
    NetData m_netdata;
    memset(&m_netdata, 0,sizeof(m_netdata));
    sprintf(m_netdata.name,"客户端%d", sockfd);
    fd_set  rfds;
    int len = 0;
    while(1){
        FD_ZERO(&rfds);
        FD_SET(0, &rfds);
        FD_SET(sockfd, &rfds);
        struct timeval tv = {1,0};
        int rst = select(sockfd+1,&rfds,NULL,NULL,&tv);

        if(rst == -1){
            printf("终止，select出错，信息：%s\n",strerror(errno));
            break;
        }
        else if (rst > 0){

            if(FD_ISSET(sockfd,&rfds)){
                // 接收数据
                NetData netdata;
                bzero(&netdata, sizeof(NetData));
                len = recv(sockfd, (void *)&netdata, sizeof(NetData), 0);
                if(len > 0){
                    printf("%s说:\n\t%s", netdata.name, netdata.buf);
                }
                else if(len < 0){
                    printf("消息接收失败,错误信息:%s\n", strerror(errno));
                    printf("终止！\n");
                    break;
                }
                else{
                    printf("牢房已关闭\n");
                    printf("终止！\n");
                    break;
                }

            }
            else if(FD_ISSET(0,&rfds)){
                // 发送数据
                bzero(m_netdata.buf,sizeof(m_netdata.buf));
                //                printf("请输入发给客户端的消息：\n");

                // 获取用户输入
                fgets(m_netdata.buf, MAX_SIZE, stdin);
                while(m_netdata.buf[0] == '\0'){
                    printf("内容不能为空!\n");
                    fgets(m_netdata.buf, MAX_SIZE, stdin);
                }
                if( !strncasecmp(m_netdata.buf, "quit", 4)){
                    printf("退出\n");
                    break;
                }

                len =  send(sockfd, (const void*)&m_netdata,sizeof(NetData),0);
                if(len <= 0){
                    printf("消息;%s 发送失败,错误信息:%s\n",m_netdata.buf,strerror(errno));
                    printf("终止！\n");
                    break;
                }
                // 客户端打印一份内容
                printf("我说:\n\t%s", m_netdata.buf);
            }
        }
    }
    FD_ZERO(&rfds);
    close(sockfd);
    return 0;

}

int dealServer(int sockfd){
    // 处理收发数据
    fd_set  rfds;
    socklen_t socklen ;

    int len = 0;
    int c_fd;
    struct sockaddr_in c_addr;
    NetData m_netdata;
    memset(&m_netdata, 0,sizeof(m_netdata));
    sprintf(m_netdata.name,"警长");
    while(1){
        struct timeval tv = {1,0};
        FD_ZERO(&rfds);
        FD_SET(0, &rfds);
        FD_SET(g_fd, &rfds);
        int max_fd = g_fd;
        if(g_clientList != NULL){
            addUserFD(g_clientList,&rfds); 
            max_fd = g_clientList->fd;
        }
        int rst = select(max_fd+1,&rfds,NULL,NULL,&tv);

        if(rst == -1){
            printf("终止，select出错，信息：%s\n",strerror(errno));
            break;
        }
        else if (rst > 0){
            CList *p = NULL;
            if(FD_ISSET(0,&rfds)){
                // 发送数据
                bzero(m_netdata.buf,sizeof(m_netdata.buf));
                //                printf("请输入发给囚犯的消息：\n");

                // 获取用户输入
                fgets(m_netdata.buf, MAX_SIZE, stdin);
                while(m_netdata.buf[0] == '\0'){
                    printf("内容不能为空!\n");
                    fgets(m_netdata.buf, MAX_SIZE, stdin);
                }
                if( !strncasecmp(m_netdata.buf, "quit", 4)){
                    printf("退出\n");
                    break;
                }

                // 遍历囚犯，发送给每一个囚犯
                CList *plt = g_clientList;
                while(plt != NULL){
                    len =  send(plt->fd, (const void*)&m_netdata,sizeof(NetData),0);
                    if(len <= 0){
                        printf("消息;%s 发送失败,错误信息:%s\n",m_netdata.buf,strerror(errno));
                        printf("终止！\n");
                    }
                    plt = plt->next;
                }

                // 服务器打印一份内容
                printf("我说:\n\t%s", m_netdata.buf);
            }
            // 新来一个囚犯
            if(FD_ISSET(g_fd,&rfds)){
                len = sizeof(struct sockaddr);
                if((c_fd = accept(sockfd, (struct sockaddr *)&c_addr, &socklen)) == -1){
                    perror("socket accept failed");
                    return 1;
                }
                // 打印囚犯的ip和port
                printf("囚犯:%d,囚犯端编码:%s,囚犯ID:%d\n",c_fd, inet_ntoa(c_addr.sin_addr), c_addr.sin_port);
                
                // 将囚犯插入链表中
                addClient(&g_clientList, c_fd);
            }
            // 处理囚犯发来的数据
            p = g_clientList;
            while(p != NULL ){
                if(FD_ISSET(p->fd, &rfds)){
                    printf("ok client: fd = %d\n",p->fd);
                    // 接收数据
                    c_fd = p->fd;
                    NetData netdata;
                    bzero(&netdata, sizeof(NetData));
                    len = recv(c_fd, (void *)&netdata, sizeof(NetData), 0);
                    sprintf(netdata.name,"囚犯%d",c_fd);
                    if(len > 0){
                        CList *plt = g_clientList;
                        while(plt != NULL){
                            if(plt->fd != c_fd){
                                send(plt->fd, (const void*)&netdata,sizeof(netdata),0);
                            } 
                            plt = plt->next;
                        }
                        printf("%s说:\n\t%s", netdata.name, netdata.buf);
                        p = p->next;
                    }
                    else if(len < 0){
                        printf("消息接收失败,错误信息:%s\n", strerror(errno));
                        p = delClientNext(&g_clientList,p);
                        printf("终止！\n");
                        continue;
                    }
                    else{
                        printf("囚犯退出！\n");
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
    printf("关闭牢房:%d\n",sockfd);
    close(sockfd);
    return 0;
}

#define MAX_NUM 5
int main(int argc, char **argv)
{
    int flag = 0;//0:表示客户端，1 表示服务器
    struct sockaddr_in addr ;
    unsigned short port = 6000, num = 5;
    char ip[16];
    strcpy(ip,"127.0.0.1");
    
    int opt;
    while((opt = getopt(argc, argv, ":p:i:n:h")) != -1){
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
            case 'h':
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
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip);
    addr.sin_port = htons(port);
   
    //第一次，以客户端开启
    
    if(connect(g_fd, (struct sockaddr *)&addr, sizeof(struct sockaddr)) == -1){
        // 连接失败，说明服务器不存在，自己作为服务器开启
        flag = 1;
    }
    
    if(flag == 1){ //作为服务器
        // 绑定socket
        if(bind(g_fd, (struct sockaddr *)&addr, sizeof(struct sockaddr)) == -1){
            perror("socket bind failed!");
            return 1;
        }

        // 监听socket
        if(listen(g_fd, num) == -1){
            perror("socket listen failed!");
            return 1;
        }

        printf("------等待囚犯建立连接请求-------\n");
        dealServer(g_fd);
    }
    else{//作为客户端
        dealClient(g_fd);
    }
    return 0;
}
