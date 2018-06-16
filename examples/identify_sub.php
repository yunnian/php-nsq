
<?php 

//sub

$nsq_lookupd = new NsqLookupd("127.0.0.1:4161"); //the nsqlookupd http addr
$identify = array(

    "client_id" => "begon",
    "deflate" => false,
    "deflate_level" => 6,
    "feature_negotiation" => true,
    "heartbeat_interval" => 60000, //  Cannot exceed  "-max-heartbeat-interval duration" configure that when the nsqd startup  
    "hostname"=>"bogon",
    "long_id"=>"bogon",
    "msg_timeout" => 800000, // you should set --msg-timeout too  when the nsqd startup
    "output_buffer_size" => 16384,
    "output_buffer_timeout" => 250,
    "sample_rate" => 0,
    "short_id" =>  "bogon",
    "snappy" => false,
    "tls_v1" => false,
    "user_agent" =>  "php-nsq/3.0"
);

$nsq = new Nsq($identify);

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
    sleep(100);
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
