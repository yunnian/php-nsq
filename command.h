void nsq_subscribe(struct bufferevent *bev, const char *topic, const char *channel);
void nsq_requeue(struct bufferevent *bev, const char *id, int timeout_ms);
/*
void nsq_ready(struct Buffer *buf, int count);
void nsq_finish(struct Buffer *buf, const char *id);
void nsq_requeue(struct Buffer *buf, const char *id, int timeout_ms);
void nsq_nop(struct Buffer *buf);
*/
