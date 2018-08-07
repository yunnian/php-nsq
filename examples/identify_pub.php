
<?php 
// pub with identify command
$nsqdAddr = array(
    "127.0.0.1:4150",
    "127.0.0.1:4154",
);

$identify = array(
    "client_id" => "begon",
    "deflate" => false,
    "deflate_level" => 6,
    "feature_negotiation" => true,
    "heartbeat_interval" => 50000,
);
$nsq = new Nsq($identify);// the same to sub
$isTrue = $nsq->connectNsqd($nsqdAddr);

for($i = 0; $i < 10; $i++){
    $msg = "nihao".$i;
    $nsq->publish("test", $msg);
}

$nsq->closeNsqdConnection();


