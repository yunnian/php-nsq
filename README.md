# php-nsq

nsq  client for php extension;



#### Quick Start :
    1. sudo phpize
    2. ./configure 
    3. make  
    4. make install  

    add in your php.ini:

    extension = nsq.so;


###### example for pub:

```

<?php 
//the nsqd tcp addr that you want to publish
$nsqd_addr = array(
    "127.0.0.1:4150",
    "127.0.0.1:4154"
);

$nsq = new Nsq();
$is_true = $nsq->connect_nsqd($nsqd_addr);

for($i=0;$i<20;$i++){
    $nsq->publish("test", "nihao");
}


```


###### example for sub: 
```
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
   echo "msg:".$msg; 

});

```

###### Dependencies:

```
libevent

```


