<?php 

//sub
$nsq_lookupd = new NsqLookupd("127.0.0.1:4161"); //the nsqlookupd tcp addr
$nsq = new Nsq();
$config = array(
    "topic"=>"test",
    "channel"=>"struggle",
    "rdy" =>2,
    "connect_num" => 1, 
);
$nsq->subscribe($nsq_lookupd, $config, function($msg){ 
   echo "msg:".$msg."\n"; 

});
