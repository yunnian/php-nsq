<?php 
//the nsqd tcp addr that you want to publish
$nsqdAddr = array(
    "101.200.220.194:4150",
//    "127.0.0.1:4154"
);

$nsq = new Nsq();
$isTrue = $nsq->connectNsqd($nsqdAddr);

for($i = 0; $i < 101; $i++){
    $msg = "nihao".$i;
    $nsq->publish("test", $msg);
}

$nsq->closeNsqdConnection();

/*
// deferred publish
$deferred = new Nsq();
$isTrue = $deferred->connectNsqd($nsqdAddr);

for($i = 0; $i < 9; $i++){

    $delayMsec = 360000;
    //deferredPublish(string topic,string message, int millisecond); 
    $deferred->deferredPublish("test", "the message will be received after".$delayMsec/(1000*60)."minutes", $delayMsec);

}
$deferred->closeNsqdConnection();
*/
