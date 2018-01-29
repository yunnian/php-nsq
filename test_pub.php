<?php 
//the nsqd tcp addr that you want to publish
$nsqdAddr = array(
    "127.0.0.1:4150",
    "127.0.0.1:4154"
);

$nsq = new Nsq();
$isTrue = $nsq->connectNsqd($nsqdAddr);

for($i = 0; $i < 9; $i++){
    $nsq->publish("test", "nihaoa");
}


// deferred publish
$deferred = new Nsq();
$isTrue = $deferred->connectNsqd($nsqdAddr);

for($i = 0; $i < 9; $i++){

    $delayMsec = 3600;
    //deferredPublish(string topic,string message, int millisecond); 
    $deferred->deferredPublish("test", "the message will be received after".$delayMsec/(1000*60)."minutes", $delayMsec);

}

//if you want set identify 
$deferred = new Nsq( [ "client_id"=>"wuzhenyu", "deflate"=>false, "deflate_level"=>6 ]);
//TODO: auth responses  and set identify for sub  
