#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include <errno.h>
#include<unistd.h>
#include <inttypes.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include"sub.h"
#include <time.h>  
#include <event2/bufferevent.h>  
#include <event2/buffer.h>  
#include <event2/listener.h>  
#include <event2/util.h>  
#include <event2/event.h>  

const int BUFFER_SIZE = 1024;  

void conn_writecb(struct bufferevent *, void *);  
void readcb(struct bufferevent *, void *msg);  
void conn_eventcb(struct bufferevent *, short, void *);  
void error_handling(char* message);

void error_handling(char* message) {
    fputs(message, stderr);
    fputc('\n', stderr);
    //exit(1);
}
int readI16(const unsigned char * pData, uint16_t *pValue)
{
    *pValue = (pData[0] << 8) | pData[1];
    return 0;
}
int readI32(const unsigned char * pData, int32_t *pValue)
{
    *pValue = (pData[0] << 24) | (pData[1] << 16) | (pData[2] << 8) | pData[3];
    return 0;
}


int readI64(const unsigned char* data, int64_t *pValue)
{
    *pValue = ((uint64_t)data[0] << 56) | ((uint64_t)data[1] << 48) | ((uint64_t)data[2] << 40) |((uint64_t)data[3] << 32)|((uint64_t)data[4] << 24) |((uint64_t)data[5] << 16) | ((uint64_t)data[6] << 8) |(uint64_t) data[7];
    return 0;

}
uint64_t ntoh64(const uint8_t *data) {
    return (uint64_t)(data[7]) | (uint64_t)(data[6])<<8 |
        (uint64_t)(data[5])<<16 | (uint64_t)(data[4])<<24 |
        (uint64_t)(data[3])<<32 | (uint64_t)(data[2])<<40 |
        (uint64_t)(data[1])<<48 | (uint64_t)(data[0])<<56;
}


int subscribe(const char *address, const char* port,struct NSQMsg *msg, int (*callback)(NSQMsg *msg)){
    struct sockaddr_in srv;  
    memset(&srv, 0, sizeof(srv));  
    int retry_num = 1;
    srv.sin_addr.s_addr = inet_addr(address);  
    srv.sin_family = AF_INET;  
    srv.sin_port = htons(atoi(port));  
    struct event_base *base = event_base_new();  
    if (!base)  
    {  
        printf("Could not initialize libevent\n");  
        return 1;  
    }  

    struct bufferevent* bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);  
    //监听终端输入事件 暂时用不上 
    //struct event* ev_cmd = event_new(base, STDIN_FILENO,  EV_READ | EV_PERSIST,  cmd_msg_cb, (void*)bev)

    NSQArg *arg ; 
    arg = malloc(sizeof(NSQArg));
    arg->msg= msg;
    arg->host = address;
    arg->port = port;
    arg->callback = callback;
    bufferevent_setcb(bev, readcb, NULL, conn_eventcb, (void *) arg);  
    int flag=bufferevent_socket_connect(bev, (struct sockaddr *)&srv,sizeof(srv));  
    bufferevent_enable(bev, EV_READ | EV_WRITE);  
    if(-1==flag)  
    {  
    
        //printf("Connect failed retry:%d\n",retry_num );  
        printf("Connect failed retryn");  
        /*
        if(retry_num <= 10000){
            retry_num ++;
            bufferevent_free(bev);  
            event_base_free(base);  
            subscribe(address, port, msg, callback);
        }
        */
        return 1;
    }  

    event_base_dispatch(base);  
    free(arg);
    event_base_free(base);  
    return 1;

}




void conn_eventcb(struct bufferevent *bev, short events, void *user_data)  
{  
    if (events & BEV_EVENT_EOF)  
    {  
        printf("Connection closed ,retrying\n");  
        //subscribe(((NSQArg *)user_data)->host, ((NSQArg *)user_data)->port,((NSQArg *)user_data)->msg, ((NSQArg *)user_data)->callback);
    }  
    else if (events & BEV_EVENT_ERROR)  
    {  
        printf("Got an error on the connection: %s, retry agin\n",strerror(errno));  
        //关闭fd 并更改状态
        sleep(1);
        bufferevent_free(bev);  
        subscribe(((NSQArg *)user_data)->host, ((NSQArg *)user_data)->port,((NSQArg *)user_data)->msg, ((NSQArg *)user_data)->callback);
    }  
    else if( events & BEV_EVENT_CONNECTED)  
    {  
        printf("Connect succeed\n");  
        struct NSQMsg *msg = ((struct NSQArg *)user_data)->msg;
        char * v  = (char * ) malloc(4);
        memcpy(v, "  V2", 4);
        //write(sock, v, 4);
        bufferevent_write(bev, v, 4);  
        free(v);
        char b[120];
        size_t n;
        //char * msg2 ="SUB test Struggle ZhenyudeMacBook-Pro ZhenyudeMacBook-Pro.local\n";
        n = sprintf(b, "SUB %s %s%s", msg->topic, msg->channel, "\n");
        //n = sprintf(b, "%s", msg2);
        //send(sock, b,strlen(msg2) ,0);
        bufferevent_write(bev, b, strlen(b));  
        char * rd =  "RDY 2\n";
        //send(sock, rd,strlen(rd) ,0);
        //客户端链接成功后
        bufferevent_write(bev, rd, strlen(rd));  
        return ;  
    }  

    bufferevent_free(bev);  
}  

void readcb(struct bufferevent *bev,void *arg){
    struct NSQMsg *msg = ((struct NSQArg *)arg)->msg;
    int (* callback)(NSQMsg *msg) = ((struct NSQArg *)arg)->callback;
    /*
       struct evbuffer *input, *output;  
       char *request_line;  
       size_t len;  
       input = bufferevent_get_input(bev);//其实就是取出bufferevent中的input  
       output = bufferevent_get_output(bev);//其实就是取出bufferevent中的output  
     */
    errno = 0;
    int i = 0;
    while(1){

        //读取msg长度
        char * msg_size = malloc(4);
        memset(msg_size,0x00,4);
        //int size_l =  read(sock, msg_size, 4);
        size_t size_l = bufferevent_read(bev, msg_size, 4); 
        readI32((const unsigned char *) msg_size ,  &msg->size);

        //读取相应长度的msg内容
        char * message = malloc(msg->size +1);
        memset(message,0x00,msg->size);
        //int l =  read(sock, message, msg->size);
        int l =  bufferevent_read(bev, message, msg->size);
        if (errno) {
            printf("errno = %d\n", errno); // errno = 33
            perror("sqrt failed"); // sqrt failed: Numerical argument out of domain
            printf("error: %s\n", strerror(errno)); // error: Numerical argument out of domain
        }
        if(l){
            msg->message_id = (char * )malloc(17);
            memset(msg->message_id,'\0',17);
            readI32((const unsigned char *)message, &msg->frame_type);


            if(msg->frame_type == 0){
                printf("%s","OK");
                if(msg->size == 15){
                    //send(sock, "NOP\n",strlen("NOP\n") ,0);
                    bufferevent_write(bev, "NOP\n",strlen("NOP\n"));
                }
            }else if(msg->frame_type == 2){
                msg->timestamp = (int64_t)ntoh64((const unsigned char *)message+4);
                readI16((const unsigned char *)message+12,  &msg->attempts);
                memcpy(msg->message_id, message+14, 16);
                msg->body = (char * )malloc(msg->size-30+1);
                memset(msg->body,'\0',msg->size-30+1);
                memcpy(msg->body,message+30, msg->size-30);
                char  ack[22] = "FIN " ;
                //strcat(ack, messageId);
                sprintf(ack,"FIN %s\n",msg->message_id);
                //send(sock, ack,strlen(ack) ,0);
                bufferevent_write(bev, ack, strlen(ack));  
                //send(sock,rd,strlen(rd) ,0);  
                //char * rd =  "RDY 2\n";
                //bufferevent_write(bev,rd, strlen(rd));  
                callback(msg);
                free(msg->body);
            }
            free(message);
            free(msg_size);
            free(msg->message_id);
        }else{
            break;

        }


        if (l == -1) {
            error_handling("read() error");;
        }
    }

    //close(sock);

    //return 0;

}
void conn_writecb(struct bufferevent *bev, void *user_data)  
{  
}  

