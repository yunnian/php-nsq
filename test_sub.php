<?php 

//sub
$nsq_lookupd = new NsqLookupd("127.0.0.1:4161"); //the nsqlookupd tcp addr
$nsq = new Nsq();
$config = array(
    "topic"=>"test",
    "channel"=>"struggle",
    "rdy" =>10,
    "connect_num" => 1, 
    "retry_delay_time" => 5000,  // after 5000 msec, message will be retried
);
$nsq->subscribe($nsq_lookupd, $config, function($msg){ 
    echo $msg->message_id."\n";
    echo $msg->attempts."\n";
    echo $msg->payload."\n";
    echo $msg->timestamp."\n";

    //if you want to retry your message:
    if($msg->attempts < 2){
        echo "llllllllll";
        throw new Exception("retry");  
    }
    return;



});
