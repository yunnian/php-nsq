#include "php.h"
#include "zend_API.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <inttypes.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>  
#include <event2/bufferevent.h>  
#include <event2/buffer.h>  
#include <event2/listener.h>  
#include <event2/util.h>  
#include <event2/event.h>  
#include"sub.h"
#include"command.h"
extern  zend_class_entry *nsq_message_ce;

const int BUFFER_SIZE = 1024;  
extern void error_handlings(char* message);

void conn_writecb(struct bufferevent *, void *);  
void readcb(struct bufferevent *, void *msg);  
void conn_eventcb(struct bufferevent *, short, void *);  
int check_ipaddr(const char *ip);

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


int subscribe(const char *address, const char* port,struct NSQMsg *msg, zend_fcall_info *fci, zend_fcall_info_cache *fcc){
    struct sockaddr_in srv;  
	struct hostent *he;
    memset(&srv, 0, sizeof(srv));  
    int retry_num = 1;

	if(check_ipaddr(address)){
        srv.sin_addr.s_addr = inet_addr(address);  
    }else{
        /* resolve hostname */
        if ( (he = gethostbyname(address) ) == NULL ) {
            exit(1); /* error */
        }
        /* copy the network address to sockaddr_in structure */
        memcpy(&srv.sin_addr, he->h_addr_list[0], he->h_length);
    }
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
    arg->fci = fci;
    arg->fcc = fcc;
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
        //subscribe(((NSQArg *)user_data)->host, ((NSQArg *)user_data)->port,((NSQArg *)user_data)->msg, fci, fcc);
    }  
    else if (events & BEV_EVENT_ERROR)  
    {  
        printf("Got an error on the connection: %s, retry agin\n",strerror(errno));  
        //关闭fd 并更改状态
        sleep(1);
        bufferevent_free(bev);  
		subscribe(((NSQArg *)user_data)->host, ((NSQArg *)user_data)->port,((NSQArg *)user_data)->msg, ((NSQArg *)user_data)->fci,((NSQArg *)user_data)->fcc);
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

        //n = sprintf(b, "SUB %s %s%s", msg->topic, msg->channel, "\n");
        //bufferevent_write(bev, b, strlen(b));  
        nsq_subscribe(bev,msg->topic, msg->channel);

        //n = sprintf(b, "%s", msg2);
        //send(sock, b,strlen(msg2) ,0);
        nsq_ready(bev, msg->rdy);
        return ;  
    }  

    bufferevent_free(bev);  
}  

void readcb(struct bufferevent *bev,void *arg){
	
    struct NSQMsg *msg = ((struct NSQArg *)arg)->msg;
	zend_fcall_info  *fci = ((struct NSQArg *)arg)->fci;;
	zend_fcall_info_cache *fcc = ((struct NSQArg *)arg)->fcc;
    errno = 0;
    int i = 0;
    while(1){

        char * msg_size = malloc(4);
        memset(msg_size,0x00,4);
        size_t size_l = bufferevent_read(bev, msg_size, 4); 
        readI32((const unsigned char *) msg_size ,  &msg->size);

        //读取相应长度的msg内容
        char * message = malloc(msg->size +1);
        memset(message,0x00,msg->size);
        //int l =  read(sock, message, msg->size);
        int l =  bufferevent_read(bev, message, msg->size);
        if (errno) {
            printf("errno = %d\n", errno); // errno = 33
            perror("sqrt failed"); 
            printf("error: %s\n", strerror(errno)); 
        }
        if(l){
            msg->message_id = (char * )malloc(17);
            memset(msg->message_id,'\0',17);
            readI32((const unsigned char *)message, &msg->frame_type);


            if(msg->frame_type == 0){
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

                zval retval;
                zval params[1];

                zend_string * body =  zend_string_init(msg->body, msg->size -30, 0);
                zval *msg_object;
                zval message_id;
                zval attempts;
                zval payload ;
                zval timestamp ;

                do{msg_object = (zval *)emalloc(sizeof(msg_object)); bzero(msg_object, sizeof(zval));}while(0);
                object_init_ex(msg_object, nsq_message_ce);

                //message_id
                zend_string *message_id_str = zend_string_init(msg->message_id, 16, 0);
                ZVAL_STR_COPY(&message_id, message_id_str);  
                zend_update_property(nsq_message_ce,msg_object,ZEND_STRL("message_id"), &message_id TSRMLS_CC);

                //attempts
                ZVAL_LONG(&attempts, msg->attempts);
                zend_update_property(nsq_message_ce,msg_object,ZEND_STRL("attempts"), &attempts TSRMLS_CC);

                //timestamp
                ZVAL_LONG(&timestamp, msg->timestamp);
                zend_update_property(nsq_message_ce,msg_object,ZEND_STRL("timestamp"), &timestamp TSRMLS_CC);

                //payload
                zend_string *payload_str = zend_string_init(msg->body, msg->size-30, 0);
                ZVAL_STR_COPY(&payload, payload_str);  
                zend_update_property(nsq_message_ce,msg_object,ZEND_STRL("payload"), &payload TSRMLS_CC);

                //call function
                ZVAL_OBJ(&params[0], Z_OBJ_P(msg_object));  
                //ZVAL_STR_COPY(&params[0], body);  
                fci->params = params;
                fci->param_count = 1;
                fci->retval = &retval;
                if(zend_call_function(fci, fcc TSRMLS_CC) !=SUCCESS){
                    //delay_time = zend_read_property(nsq_ce, getThis(), "retry_delay_time", sizeof("retry_delay_time")-1, 1, &rv3);
                    nsq_requeue(bev, msg->message_id, msg->delay_time);
                }else{
                    nsq_finish(bev, msg->message_id);
                }

                //free memory
                zval_dtor(params);
                zend_string_release(body);
                zend_string_release(payload_str);
                zend_string_release(message_id_str);
                zval_dtor(&timestamp);
                zval_dtor(&message_id);
                zval_dtor(&attempts);
                zval_dtor(&payload);
                efree(msg_object);
                free(msg->body);
            }
            free(message);
            free(msg_size);
            free(msg->message_id);
        }else{
            break;

        }

        if (l == -1) {
            //error_handlings("read() error");;
        }
    }

    //close(sock);

    //return 0;
}
int check_ipaddr (const char *str) 
{
	if (str == NULL || *str == '\0'){
		return 0;
	}

	struct sockaddr_in6 addr6; 
	struct sockaddr_in addr4; 

	if (1 == inet_pton (AF_INET, str, &addr4.sin_addr)){
		return 1;
	} else if (1 == inet_pton (AF_INET6, str, &addr6.sin6_addr)){
		return 1;
	}
	return 0;
}

void conn_writecb(struct bufferevent *bev, void *user_data)  
{  
}  

