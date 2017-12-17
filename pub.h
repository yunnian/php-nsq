//
// Created by Zhenyu Wu on 2017/9/14.
//

#ifndef STRUGGLE_NSQ_PUB_CLIENT_C_H
#define STRUGGLE_NSQ_PUB_CLIENT_C_H
typedef struct nsqd_connect_config {
     char * host; 
     char * port; 

}nsqd_connect_config;
int* connect_nsqd(nsqd_connect_config * connect_config_arr, int connect_num);
int publish(int sock, char *topic, char *msg);

#endif //STRUGGLE_NSQ_PUB_CLIENT_C_H
