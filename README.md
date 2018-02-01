# php-nsq

nsq  client for php72 extension;


### intall :

    Dependencies: libevent

    1. sudo phpize
    2. ./configure 
    3. make  
    4. make install  

    add in your php.ini:

    extension = nsq.so;


### Example for pub:

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


// Deferred publish 
//function : deferredPublish(string topic,string message, int millisecond); 
//millisecond default : [0 < millisecond < 3600000]

$deferred = new Nsq();
$isTrue = $deferred->connectNsqd($nsqdAddr);
for($i = 0; $i < 20; $i++){
    $deferred->deferredPublish("test", "message daly", 3000); 
}

```

### Example for sub:
```
<?php 

//sub
ini_set('memory_limit', '-1');

$nsq_lookupd = new NsqLookupd("127.0.0.1:4161"); //the nsqlookupd tcp addr
$nsq = new Nsq();
$config = array(
    "topic" => "test",
    "channel" => "struggle",
    "rdy" => 2,                //optional , default 1
    "connect_num" => 1,        //optional , default 1   
    "retry_delay_time" => 5000,  //optional, default 0 , if run callback failed, after 5000 msec, message will be retried
);

$nsq->subscribe($nsq_lookupd, $config, function($msg,$bev){ 

    echo $msg->payload;
    echo $msg->attempts;
    echo $msg->message_id;
    echo $msg->timestamp;


});

```
### Nsq Object

* `publish($topic,$channel)` <br/>

* `deferredPublish($topic,$channel,$msec)` <br/>

* `subscribe($nsq_lookupd,$config,$callback)` <br/>

### Message Object

The following properties and methods are available on Message objects produced by a Reader
instance.

* `timestamp` <br/>
  Numeric timestamp for the Message provided by nsqd.
* `attempts` <br/>
  The number of attempts that have been made to process this message.
* `message_id` <br/>
  The opaque string id for the Message provided by nsqd.
* `payload` <br/>
  The message payload as a Buffer object.
* `finish($bev,$msg->message_id)` <br/>
  Finish the message as successful.
* `touch($bev,$msg->message_id)` <br/>
  Tell nsqd that you want extra time to process the message. It extends the
  soft timeout by the normal timeout amount.



### tips :

```
1. requeue/retry:

if you whant to retry your mesage when callback have something wrong, just throw any Exception , example:

<?php 

$nsq->subscribe($nsq_lookupd, $config, function($msg){ 

    //do something , error or call something timeout ,you can retry your message:

    if($msg->attempts < 3){
        //the message will be retried after you configure retry_delay_time 
        throw new Exception(""); 
    }else{
        return;
    }

});


2. if your have strong consuming ability ,you can add you rdy num and connect num


3. you can use supervisor to supervise process


```

Changes
-------
* **2.2.0**
  * Fix pub bug zend_mm_heap corrupted 
  * Fix pub block bug  when received the 'heartbeats' 
  * Add the bufferevent resource
  * Add the deferred publish
  * Add the touch function
  * Add the finish function
* **2.1.1**
  * Fix core dump
* **2.0**
  * retry
  * message object
  * fix c99 install error
  * license
