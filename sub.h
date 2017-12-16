
//
// Created by Zhenyu Wu on 2017/9/13.
//

#ifndef STRUGGLE_NSQ_SUB_CLIENT_H
#define STRUGGLE_NSQ_SUB_CLIENT_H

typedef struct NSQMsg {
    const char * topic;
    const char * channel;
    int32_t frame_type;
    int64_t timestamp;
    uint16_t attempts;
    char *message_id;
    int32_t size;
    char *body;
    int rdy;
}NSQMsg;

typedef struct NSQArg{
    NSQMsg *msg;
    const char * host;
    const char * port;
	zend_fcall_info  *fci;
	zend_fcall_info_cache *fcc;
	
}NSQArg;
//param is the nsqlookeupd's ip and port ,return the socket fd
int subscribe(const char *address, const char * port, struct NSQMsg *msg, zend_fcall_info *fci, zend_fcall_info_cache *fcc);

#endif //STRUGGLE_NSQ_SUB_CLIENT_H
