<?php 


$nsq_lookupd = new NsqLookupd("127.0.0.1:4161");
$nsq = new Nsq();
$config = [

    "topic"=>"test",
    "channel"=>"struggle",
    "rdy" => 10

    ];
function nihao($msg){
   echo "msg:".$msg; 

}
//$nsq->subscribe($config,["nihao","h"]);
//$nsq->subscribe($config);
$nsq->subscribe($nsq_lookupd, $config, function($msg){ 
   echo "msg:".$msg; 

});
