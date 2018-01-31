<?php 

//sub

$nsq_lookupd = new NsqLookupd("127.0.0.1:4161"); //the nsqlookupd tcp addr
$nsq = new Nsq();

$config = array(
    "topic" => "test",
    "channel" => "struggle",
    "rdy" => 10,
    "connect_num" => 1, 
    "retry_delay_time" => 5000,  // after 5000 msec, message will be retried
);

$nsq->subscribe($nsq_lookupd, $config, function($msg,$bev){ 

    //$msg->touch($bev,$msg->message_id);

    echo $msg->payload.$msg->attempts."\n";
});
