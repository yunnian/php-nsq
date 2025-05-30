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

extern int le_bufferevent;

// IPC communication structures
typedef struct {
    char message_id[17];
    char *body;
    size_t body_len;
    int64_t timestamp;
    uint16_t attempts;
    int delay_time;
    int auto_finish;
} worker_message_t;

typedef struct {
    char message_id[17];
    int success; // 1 = success, 0 = failed
} worker_result_t;

// Global pipes
static int msg_pipe[2];  // main process -> worker process
static int result_pipe[2]; // worker process -> main process
static pid_t worker_pid = 0;

// Event handler for result_pipe
void result_pipe_cb(evutil_socket_t fd, short events, void *arg) {
    struct NSQArg *nsq_arg = (struct NSQArg *)arg;
    struct bufferevent *bev = (struct bufferevent *)nsq_arg->bev_res->ptr;
    struct NSQMsg *msg = nsq_arg->msg;
    int auto_finish = msg->auto_finish;
    
    worker_result_t result;
    ssize_t n = read(result_pipe[0], &result, sizeof(worker_result_t));
    if (n == sizeof(worker_result_t)) {
        // Send FIN/REQ based on worker processing result
        if (auto_finish) {
            if (result.success) {
                char fin_cmd[64];
                snprintf(fin_cmd, sizeof(fin_cmd), "FIN %s\n", result.message_id);
                bufferevent_write(bev, fin_cmd, strlen(fin_cmd));
            } else {
                char req_cmd[128];
                snprintf(req_cmd, sizeof(req_cmd), "REQ %s %d\n", 
                       result.message_id, msg->delay_time);
                bufferevent_write(bev, req_cmd, strlen(req_cmd));
            }
        }
        // Send RDY immediately after FIN/REQ to prepare for next message
        nsq_ready(bev, msg->rdy);
    }
}

// Worker process main loop
void worker_process_main(zend_fcall_info *fci, zend_fcall_info_cache *fcc, zend_resource *bev_res) {
    close(msg_pipe[1]);    // Close write end
    close(result_pipe[0]); // Close read end
    
    while (1) {
        worker_message_t work_msg;
        ssize_t n = read(msg_pipe[0], &work_msg, sizeof(worker_message_t));
        if (n != sizeof(worker_message_t)) {
            if (n == 0) {
                break; // pipe closed, exiting
            }
            continue; // 读取失败，继续等待
        }
        
        // 读取消息体
        char *msg_body = malloc(work_msg.body_len + 1);
        n = read(msg_pipe[0], msg_body, work_msg.body_len);
        if (n != work_msg.body_len) {
            free(msg_body);
            continue;
        }
        msg_body[work_msg.body_len] = '\0';
        
        // 创建PHP对象并调用用户函数
        zval retval;
        zval params[2];
        zval msg_object;
        zval message_id;
        zval attempts;
        zval payload;
        zval timestamp;

        object_init_ex(&msg_object, nsq_message_ce);

        //message_id
        zend_string *message_id_str = zend_string_init(work_msg.message_id, 16, 0);
        ZVAL_STR_COPY(&message_id, message_id_str);
        zend_update_property(nsq_message_ce, NSQ_COMPAT_OBJ_P(&msg_object), ZEND_STRL("message_id"), &message_id);
        zend_update_property(nsq_message_ce, NSQ_COMPAT_OBJ_P(&msg_object), ZEND_STRL("messageId"), &message_id);

        //attempts
        ZVAL_LONG(&attempts, work_msg.attempts);
        zend_update_property(nsq_message_ce, NSQ_COMPAT_OBJ_P(&msg_object), ZEND_STRL("attempts"), &attempts);
        //timestamp
        ZVAL_LONG(&timestamp, work_msg.timestamp);
        zend_update_property(nsq_message_ce, NSQ_COMPAT_OBJ_P(&msg_object), ZEND_STRL("timestamp"), &timestamp);

        //payload
        zend_string *payload_str = zend_string_init(msg_body, work_msg.body_len, 0);
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

        // 发送结果回主进程
        worker_result_t result;
        memcpy(result.message_id, work_msg.message_id, 16);
        result.message_id[16] = '\0';
        result.success = callback_success;
        
        write(result_pipe[1], &result, sizeof(worker_result_t));

        //free memory
        zval_dtor(&params[0]);
        zend_string_release(payload_str);
        zend_string_release(message_id_str);
        zval_dtor(&timestamp);
        zval_dtor(&retval);
        zval_dtor(&message_id);
        zval_dtor(&attempts);
        zval_dtor(&payload);
        free(msg_body);
    }
}

int subscribe(NSQArg *arg) {
    struct sockaddr_in srv;
    struct hostent *he;
    memset(&srv, 0, sizeof(srv));
    int retry_num = 1;

    // Create IPC pipes
    if (pipe(msg_pipe) == -1 || pipe(result_pipe) == -1) {
        perror("pipe creation failed");
        return 1;
    }

    // Fork worker process
    worker_pid = fork();
    if (worker_pid == 0) {
        // Worker process
        worker_process_main(arg->fci, arg->fcc, arg->bev_res);
        exit(0); // Worker process exit
    } else if (worker_pid < 0) {
        perror("fork failed");
        return 1;
    }

    // Main process: close unnecessary pipe ends
    close(msg_pipe[0]);    // Close read end
    close(result_pipe[1]); // Close write end

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
    
    // 添加result_pipe的事件监听
    struct event *result_event = event_new(base, result_pipe[0], EV_READ | EV_PERSIST, result_pipe_cb, (void *)arg);
    event_add(result_event, NULL);
    
    if (-1 == flag) {
        throw_exception(PHP_NSQ_ERROR_CONNECTION_FAILED);
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
    
    // 清理worker进程
    if (worker_pid > 0) {
        kill(worker_pid, SIGTERM);
        waitpid(worker_pid, NULL, 0);
    }
    close(msg_pipe[1]);
    close(result_pipe[0]);
    
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

struct NSQMsg *msg ;
int is_first = 1;
int l = 0;
char *message ;

void readcb(struct bufferevent *bev, void *arg) {
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
                // this is heartbeat - 立即响应，绝对优先
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
                // 业务消息 - 发送给worker进程处理
                
                msg->message_id = (char *) emalloc(17);
                memset(msg->message_id, '\0', 17);

                msg->timestamp = (int64_t) ntoh64((const unsigned char *) message + 4);
                readI16((const unsigned char *) message + 12, &msg->attempts);

                memcpy(msg->message_id, message + 14, 16);

                msg->body = (char *) emalloc(msg->size - 30 + 1);
                memset(msg->body, '\0', msg->size - 30 + 1);
                memcpy(msg->body, message + 30, msg->size - 30);

                // 发送消息给worker进程处理
                worker_message_t work_msg;
                memcpy(work_msg.message_id, msg->message_id, 16);
                work_msg.message_id[16] = '\0';
                work_msg.body_len = strlen(msg->body);
                work_msg.timestamp = msg->timestamp;
                work_msg.attempts = msg->attempts;
                work_msg.delay_time = msg->delay_time;
                work_msg.auto_finish = auto_finish;

                // 发送消息头
                ssize_t n = write(msg_pipe[1], &work_msg, sizeof(worker_message_t));
                if (n == sizeof(worker_message_t)) {
                    // 发送消息体
                    write(msg_pipe[1], msg->body, work_msg.body_len);
                }

                // 清理主进程的内存
                efree(msg->body);
                efree(msg->message_id);

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
            error_handlings("read() error");;
        }

    }
}

void conn_writecb(struct bufferevent *bev, void *user_data) {
}  

