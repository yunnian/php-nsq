<?php 

//sub
$nsq_lookupd = new NsqLookupd("127.0.0.1:4161"); //the nsqlookupd tcp addr
$nsq = new Nsq();
$config = array(
    "topic"=>"test",
    "channel"=>"struggle",
    "rdy" =>10,
    "connect_num" => 5, 
);
$nsq->subscribe($nsq_lookupd, $config, function($msg){ 
    echo $msg->message_id."\n";
    echo $msg->attempts."\n";
    echo $msg->payload."\n";
    echo $msg->timestamp."\n";

});
