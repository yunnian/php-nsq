<?php 

//sub

$nsq_lookupd = new NsqLookupd("127.0.0.1:4161"); //the nsqlookupd http addr
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

    try{
        echo $msg->payload . " " . "attempts:".$msg->attempts."\n";
        //do something
    }catch(Exception $e){

        if($msg->attempts < 3){
            //the message will be retried after you configure retry_delay_time 
            throw new Exception(""); 
        }else{
            echo $e->getMessage();
            return;
        }
    }
    //$msg->touch($bev,$msg->message_id); //if you callback run long time ,you can use this function 

});
