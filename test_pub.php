<?php 
//the nsqd tcp addr that you want to publish
$nsqdAddr = array(
    "127.0.0.1:4150",
    "127.0.0.1:4154"
);

$nsq = new Nsq();
$isTrue = $nsq->connectNsqd($nsqdAddr);

for($i = 0; $i < 9; $i++){
    $nsq->publish("test", "nihao");
}


// deferred publish
$deferred = new Nsq(["max-req-timeout"=>60000000]);
$isTrue = $deferred->connectNsqd($nsqdAddr);

for($i = 0; $i < 9; $i++){

    $delayMsec = 36000000;
    //deferredPublish(string topic,string message, int millisecond); 
    $deferred->deferredPublish("test", "the message will be received after".$delayMsec/(1000*60)."minutes", $delayMsec);

}

