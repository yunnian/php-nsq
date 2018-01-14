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
$deferred = new Nsq();
$isTrue = $deferred->connectNsqd($nsqdAddr);

for($i = 0; $i < 9; $i++){

    $dalyMsec = 3000;
    //deferredPublish(string topic,string message, int millisecond); 
    $deferred->deferredPublish("test", "the message will be received after".$dalyMsec/(1000*60)."minutes", $dalyMsec);

}

