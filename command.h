#ifndef STRUGGLE_NSQ_MESSAGE_CLIENT_C_H
#define STRUGGLE_NSQ_MESSAGE_CLIENT_C_H char * request(char * url);
void nsq_subscribe(struct bufferevent *bev, const char *topic, const char *channel);
void nsq_requeue(struct bufferevent *bev, const char *id, int timeout_ms);
void nsq_finish(struct bufferevent *bev, const char *id);
void nsq_ready(struct bufferevent *bev, int count);
void nsq_nop(struct bufferevent *bev);
#endif
