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
$nsqd_addr = array(
    "127.0.0.1:4150",
    "127.0.0.1:4154"
);

$nsq = new Nsq();
$is_true = $nsq->connect_nsqd($nsqd_addr);

for($i = 0; $i < 20; $i++){
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
    "topic" => "test",
    "channel" => "struggle",
    "rdy" => 2,                //optional , default 1
    "connect_num" => 1,        //optional , default 1   
    "retry_delay_time" => 5000,  //optional, default 0 , after 5000 msec, message will be retried
);
$nsq->subscribe($nsq_lookupd, $config, function($msg){ 
   echo "msg:".$msg; 

});

```

###### tips :


```
1. requeue/retry:

if you whant to retry your mesage when callback have something wrong, just throw any Exception , example:

<?php 

//sub
$nsq_lookupd = new NsqLookupd("127.0.0.1:4161"); //the nsqlookupd tcp addr
$nsq = new Nsq();
$config = array(
    "topic" => "test",
    "channel" => "struggle",
    "rdy" => 2,                //optional , default 1
    "connect_num" => 1,        //optional , default 1   
    "retry_delay_time" => 5000,  //optional, default 0 , after 5000 msec, message will be retried
);
$nsq->subscribe($nsq_lookupd, $config, function($msg){ 
    //do something , error or call something timeout ,you can retry your message:
    throw new Exception(""); //the message will be retried after 5 seconds

});

2.  if your have strong consuming ability ,you can add you rdy num and connect num


```

###### Dependencies:

```
libevent

```


