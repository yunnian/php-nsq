<?php 
//the nsqd tcp addr that you want to publish
$nsqd_addr = array(
    "127.0.0.1:4150",
    "127.0.0.1:4154"
);

$nsq = new Nsq();
$is_true = $nsq->connect_nsqd($nsqd_addr);

for($i = 0; $i < 1; $i++){
    $nsq->publish("test", "nihao");
}
