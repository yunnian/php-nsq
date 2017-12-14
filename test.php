<?php 
$nsq_lookupd = new NsqLookupd();
$nsq = new Nsq();
$config = ["topic"=>"test","channel"=>"struggle"];
function nihao($msg){
   echo "msg:".$msg; 

}
//$nsq->subscribe($config,["nihao","h"]);
//$nsq->subscribe($config);
$nsq->subscribe($nsq_lookupd, $config, function($msg){ 
   echo "msg:".$msg; 

});
