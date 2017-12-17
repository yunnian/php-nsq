<?php 
$nsqd_addr = array(
    "127.0.0.1:4150",
    "127.0.0.1:4154"
);

$nsq = new Nsq();


for($i=0;$i<1;$i++){
    $nsq->publish($nsqd_addr, "test", "nihao");

}
