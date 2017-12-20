#include "php.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "pub.h"

extern void error_handlings(char* message) ;

int ReadI32(const char * pData, int *pValue)
{
    *pValue = (pData[0] << 24) | (pData[1] << 16) | (pData[2] << 8) | pData[3];
    return 0;
}


static int * sock_arr;
int* connect_nsqd(nsqd_connect_config * connect_config_arr, int connect_num){
    if(sock_arr){
        return sock_arr;
    }

    sock_arr = emalloc(connect_num*sizeof(int));

    for (int i = 0; i < connect_num; i++) {
        struct sockaddr_in serv_addr;
        memset(&serv_addr, 0, sizeof(serv_addr));
        //创建用于internet的流协议(TCP)socket
        sock_arr[i] = socket(PF_INET, SOCK_STREAM, 0);
        if (sock_arr[i] == -1) {
            error_handlings("socket() error");
        }
        
        //设置一个socket地址结构client_addr,代表客户机internet地址, 端口
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = inet_addr(connect_config_arr->host);
        serv_addr.sin_port = htons(atoi(connect_config_arr->port));
        if(i < connect_num-1){
            efree(connect_config_arr->host);
            efree(connect_config_arr->port);
            connect_config_arr-- ;
        }

        //把socket和socket地址结构联系起来
        if( connect(sock_arr[i],(struct sockaddr*)&serv_addr,sizeof(serv_addr)) == -1) {
            error_handlings("connect() error");
        }
        char * msgs  = (char * ) malloc(4);
        memcpy(msgs, "  V2", 4);
        write((sock_arr[i]), msgs, 4);  
        free(msgs);
    }
    return sock_arr;
}


int publish(int sock, char *topic, char *msg){
	char buf[1024*1024];
    size_t n;
	char * pub_command = malloc(strlen(topic) + strlen("PUB \n"));
    sprintf(pub_command, "%s%s%s", "PUB ",topic, "\n");
	int  len = htonl(strlen(msg));
    n = sprintf(buf, "%s", pub_command);
	memcpy(&buf[strlen(pub_command)], &len, 4);
    n = sprintf(&buf[strlen(pub_command)+4], "%s", msg);
	int sendLen = strlen(pub_command) + strlen(msg)+4;
	send(sock, buf,sendLen ,0);  

	char message[30];
	int str_len;
    while(1) {
        int l = read(sock, &message, 2);
        if(strcmp(message,"OK")==0){
            break;
        }
        if(l == 0){
            break;
        }
    }

    if(strcmp(message,"OK")==0){
        return 0;
    }else{
        return -1;
    }
}
