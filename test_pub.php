<?php 
$nsqd_addr = array(
    "127.0.0.1:4150",
    "127.0.0.1:4154"
);

$nsq = new Nsq();
$nsq->connect_nsqd($nsqd_addr);
var_dump($nsq->nsqd_connection_fds);

/*
for($i=0;$i<20;$i++){
    $a = $nsq->publish($nsqd_addr, "test", "nihao");
    var_dump($a);

}
*/
