<?php 

//sub

$startMemory = memory_get_usage();
$nsq_lookupd = new NsqLookupd("127.0.0.1:4161"); //the nsqlookupd tcp addr
$nsq = new Nsq();

$config = array(
    "topic" => "test",
    "channel" => "struggle",
    "rdy" => 10,
    "connect_num" => 1, 
    "retry_delay_time" => 5000,  // after 5000 msec, message will be retried
    "auto_finish" => true,
);

$nsq->subscribe($nsq_lookupd, $config, function($msg,$bev){

    echo $msg->payload . " " . "attempts:".$msg->attempts."\n";
    //$msg->touch($bev,$msg->message_id); //if you callback run long time ,you can use this function 

});
