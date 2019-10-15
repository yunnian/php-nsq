<?php 
//the nsqd tcp addr that you want to publish
$nsqdAddr = array(
    "127.0.0.1:4150", //tcp addr 
    //"127.0.0.1:4154", if you have another nsqd addr
);
$nsq = new Nsq();
$isTrue = $nsq->connectNsqd($nsqdAddr);

for($i = 0; $i < 1000; $i++){
    $msg = "nihao".$i;
    $nsq->publish("test", $msg);
}

$nsq->closeNsqdConnection();
