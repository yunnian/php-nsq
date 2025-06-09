/*
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2017 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Zhenyu Wu      <wuzhenyu@kuangjue.com>                       |
  +----------------------------------------------------------------------+
*/

#include "php.h"
#include <arpa/inet.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <event2/event.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include "sub.h"
#include "command.h"
#include "common.h"
#include "ext/standard/php_var.h"
#include "nsq_exception.h"
#include "zend_exceptions.h"

extern zend_class_entry *nsq_message_ce;

extern void error_handlings(char *message);

void conn_writecb(struct bufferevent *, void *);

void readcb(struct bufferevent *, void *msg);

void conn_eventcb(struct bufferevent *, short, void *);

void cleanup_message_queue();

extern int le_bufferevent;

// Message queue structure - for caching business messages to be processed
typedef struct pending_message {
    char message_id[17];
    char *body;
    size_t body_len;
    int64_t timestamp;
    uint16_t attempts;
    int delay_time;
    int auto_finish;
    struct pending_message *next;
} pending_message_t;

// Global message queue
static pending_message_t *message_queue_head = NULL;
static pending_message_t *message_queue_tail = NULL;
static int pending_message_count = 0;

// Add message to queue
void enqueue_message(const char *message_id, const char *body, size_t body_len, 
                    int64_t timestamp, uint16_t attempts, int delay_time, int auto_finish) {
    pending_message_t *msg = malloc(sizeof(pending_message_t));
    if (!msg) return; // Memory allocation check
    
    memcpy(msg->message_id, message_id, 16);
    msg->message_id[16] = '\0';
    msg->body = malloc(body_len + 1);
    if (!msg->body) {
        free(msg);
        return; // Memory allocation check
    }
    
    memcpy(msg->body, body, body_len);
    msg->body[body_len] = '\0';
    msg->body_len = body_len;
    msg->timestamp = timestamp;
    msg->attempts = attempts;
    msg->delay_time = delay_time;
    msg->auto_finish = auto_finish;
    msg->next = NULL;
    
    if (message_queue_tail) {
        message_queue_tail->next = msg;
        message_queue_tail = msg;
    } else {
        message_queue_head = message_queue_tail = msg;
    }
    pending_message_count++;
}

// Get message from queue
pending_message_t* dequeue_message() {
    if (!message_queue_head) return NULL;
    
    pending_message_t *msg = message_queue_head;
    message_queue_head = msg->next;
    if (!message_queue_head) {
        message_queue_tail = NULL;
    }
    pending_message_count--;
    
    return msg;
}

// Function to process single business message
void process_business_message(pending_message_t *msg, zend_fcall_info *fci, zend_fcall_info_cache *fcc, 
                             zend_resource *bev_res, struct bufferevent *bev, struct NSQMsg *nsq_msg) {
    
    // Create PHP object and call user function
    zval retval;
    zval params[2];
    zval msg_object;
    zval message_id;
    zval attempts;
    zval payload;
    zval timestamp;

    object_init_ex(&msg_object, nsq_message_ce);

    //message_id
    zend_string *message_id_str = zend_string_init(msg->message_id, 16, 0);
    ZVAL_STR_COPY(&message_id, message_id_str);
    zend_update_property(nsq_message_ce, NSQ_COMPAT_OBJ_P(&msg_object), ZEND_STRL("message_id"), &message_id);
    zend_update_property(nsq_message_ce, NSQ_COMPAT_OBJ_P(&msg_object), ZEND_STRL("messageId"), &message_id);

    //attempts
    ZVAL_LONG(&attempts, msg->attempts);
    zend_update_property(nsq_message_ce, NSQ_COMPAT_OBJ_P(&msg_object), ZEND_STRL("attempts"), &attempts);
    //timestamp
    ZVAL_LONG(&timestamp, msg->timestamp);
    zend_update_property(nsq_message_ce, NSQ_COMPAT_OBJ_P(&msg_object), ZEND_STRL("timestamp"), &timestamp);

    //payload
    zend_string *payload_str = zend_string_init(msg->body, msg->body_len, 0);
    ZVAL_STR_COPY(&payload, payload_str);
    zend_update_property(nsq_message_ce, NSQ_COMPAT_OBJ_P(&msg_object), ZEND_STRL("payload"), &payload);

    //call function
    ZVAL_OBJ(&params[0], Z_OBJ(msg_object));
    ZVAL_RES(&params[1], bev_res);
    fci->params = params;
    fci->param_count = 2;
    fci->retval = &retval;
    
    int callback_success = 0;
    if (zend_call_function(fci, fcc) == SUCCESS) {
        if (!EG(exception)) {
            callback_success = 1;
        } else {
            zend_clear_exception();
        }
    }

    // Send FIN/REQ based on callback result
    if (msg->auto_finish) {
        if (callback_success) {
            char fin_cmd[64];
            snprintf(fin_cmd, sizeof(fin_cmd), "FIN %s\n", msg->message_id);
            bufferevent_write(bev, fin_cmd, strlen(fin_cmd));
        } else {
            char req_cmd[128];
            snprintf(req_cmd, sizeof(req_cmd), "REQ %s %d\n", msg->message_id, msg->delay_time);
            bufferevent_write(bev, req_cmd, strlen(req_cmd));
        }
    }

    //free memory
    zval_dtor(&params[0]);
    zend_string_release(payload_str);
    zend_string_release(message_id_str);
    zval_dtor(&timestamp);
    zval_dtor(&retval);
    zval_dtor(&message_id);
    zval_dtor(&attempts);
    zval_dtor(&payload);
}

// Message processing event callback - use timer to periodically process messages in queue
void process_message_queue(evutil_socket_t fd, short events, void *arg) {
    struct NSQArg *nsq_arg = (struct NSQArg *)arg;
    struct bufferevent *bev = (struct bufferevent *)nsq_arg->bev_res->ptr;
    struct NSQMsg *nsq_msg = nsq_arg->msg;
    
    // Process at most one message at a time to ensure timely heartbeat processing
    pending_message_t *msg = dequeue_message();
    if (msg) {
        process_business_message(msg, nsq_arg->fci, nsq_arg->fcc, nsq_arg->bev_res, bev, nsq_msg);
        
        // Clean up message
        free(msg->body);
        free(msg);
        
        // Send RDY to prepare for next message
        nsq_ready(bev, nsq_msg->rdy);
    }
}

int subscribe(NSQArg *arg) {
    
    struct sockaddr_in srv;
    struct hostent *he;
    memset(&srv, 0, sizeof(srv));
    int retry_num = 1;

    if (check_ipaddr(arg->host)) {
        srv.sin_addr.s_addr = inet_addr(arg->host);
    } else {
        /* resolve hostname */
        if ((he = gethostbyname(arg->host)) == NULL) {
            exit(1); /* error */
        }
        /* copy the network address to sockaddr_in structure */
        memcpy(&srv.sin_addr, he->h_addr_list[0], he->h_length);
    }
    srv.sin_family = AF_INET;
    srv.sin_port = htons(atoi(arg->port));
    struct event_base *base = event_base_new();
    if (!base) {
        throw_exception(PHP_NSQ_ERROR_LIBEVENT_COULD_NOT_BE_INITIALIZED);
        return 1;
    }

    struct bufferevent *bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);
    arg->bev_res =  zend_register_resource(bev, le_bufferevent);

    bufferevent_setcb(bev, readcb, NULL, conn_eventcb, (void *) arg);
    int flag = bufferevent_socket_connect(bev, (struct sockaddr *) &srv, sizeof(srv));
    bufferevent_enable(bev, EV_READ | EV_WRITE);
    
    // Add timer to periodically process message queue (0ms interval, fastest response)
    struct timeval tv = {0, 1000}; // 1000 milliseconds - fastest processing
    struct event *process_event = event_new(base, -1, EV_PERSIST, process_message_queue, (void *)arg);
    event_add(process_event, &tv);
    
    if (-1 == flag) {
        throw_exception(PHP_NSQ_ERROR_CONNECTION_FAILED);
        return 1;
    }

    event_base_dispatch(base);
    
    // Cleanup on exit
    cleanup_message_queue();
    event_base_free(base);
    return 1;
}

void conn_eventcb(struct bufferevent *bev, short events, void *user_data) {
    if (events & BEV_EVENT_EOF) {
        printf("Connection closed ,retrying\n");
        subscribe((NSQArg *) user_data);
    } else if (events & BEV_EVENT_ERROR) {
        printf("Got an error on the connection: %s, retry agin\n", strerror(errno));
        //close fd
        sleep(1);
        bufferevent_free(bev);
        subscribe((NSQArg *) user_data);
    } else if (events & BEV_EVENT_CONNECTED) {
        struct NSQMsg *msg = ((struct NSQArg *) user_data)->msg;
        char *v = (char *) emalloc(4);
        memcpy(v, "  V2", 4);
        evutil_socket_t fd = bufferevent_getfd(bev);
        int res = write(fd, v, 4);
        efree(v);
        send_identify(((struct NSQArg *) user_data)->nsq_obj, fd);

        nsq_subscribe(bev, msg->topic, msg->channel);
        nsq_ready(bev, msg->rdy);

        return;
    }

    bufferevent_free(bev);
}

void readcb(struct bufferevent *bev, void *arg) {
    static struct NSQMsg *msg = NULL;
    static int is_first = 1;
    static int l = 0;
    static char *message = NULL;
    
    msg = ((struct NSQArg *) arg)->msg;
    int auto_finish = msg->auto_finish;
    zend_fcall_info *fci = ((struct NSQArg *) arg)->fci;;
    zend_fcall_info_cache *fcc = ((struct NSQArg *) arg)->fcc;
    errno = 0;
    int i = 0;
    
    while (1){

        if(is_first){
            char *msg_size = emalloc(4);
            memset(msg_size, 0x00, 4);
            size_t size_l = bufferevent_read(bev, msg_size, 4);
            readI32((const unsigned char *) msg_size, &msg->size);

            message = emalloc(msg->size + 1);
            memset(message, 0x00, msg->size);
            efree(msg_size);
        }

        l += bufferevent_read(bev, message + l, msg->size - l );

        if(l < msg->size){

            is_first = 0;
            break;
        }

        if (errno) {
            //printf("errno = %d\n", errno); // errno = 33
            //printf("error: %s\n", strerror(errno));
        }
        if (l == msg->size) {
            readI32((const unsigned char *) message, &msg->frame_type);

            if (msg->frame_type == 0) {
                // Heartbeat message - respond immediately
                if (msg->size == 15) {
                    bufferevent_write(bev, "NOP\n", strlen("NOP\n"));
                    // this is response  OK
                }else if (msg->size == 6){
                    //nothing
                }
                l = 0;
                is_first = 1;
                efree(message);
                if(msg->size !=0){
                    memset(&msg->size, 0x00, 4);
                    continue;
                }else{
                    break;
                }
                break;
            } else if (msg->frame_type == 2) {
                // Business message - put into queue for processing
                
                char message_id[17];
                memset(message_id, '\0', 17);

                int64_t timestamp = (int64_t) ntoh64((const unsigned char *) message + 4);
                uint16_t attempts;
                readI16((const unsigned char *) message + 12, &attempts);

                memcpy(message_id, message + 14, 16);

                char *body = (char *) malloc(msg->size - 30 + 1);
                if (body) {  // Memory allocation check
                    memset(body, '\0', msg->size - 30 + 1);
                    memcpy(body, message + 30, msg->size - 30);

                    // Put message into queue
                    enqueue_message(message_id, body, msg->size - 30, timestamp, attempts, 
                                   msg->delay_time, auto_finish);

                    // Free temporary memory
                    free(body);
                }

                memset(&msg->size, 0x00, 4);
                efree(message);
                l = 0;
                is_first = 1;
            }
        } else {
            memset(&msg->size, 0x00, 4);
            efree(message);
            l = 0;
            is_first = 1;
            break;
        }
        if (l == -1) {
            error_handlings("read() error");
            l = 0;
            is_first = 1;
            break;
        }

    }
}

void conn_writecb(struct bufferevent *bev, void *user_data) {
}  

// Clean up all messages in queue (for shutdown)
void cleanup_message_queue() {
    while (message_queue_head) {
        pending_message_t *msg = dequeue_message();
        if (msg) {
            free(msg->body);
            free(msg);
        }
    }
}

