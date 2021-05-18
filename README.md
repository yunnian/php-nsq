# php-nsq

NSQ  client for php7 .  QQ Group : 616063018<br/>

### Changes

**2020-05-07**
- fixed https://github.com/yunnian/php-nsq/issues/39
message are binary-safe now

### install :

    Dependencies: libevent  (apt-get install libevent-dev ,yum install libevent-devel)

    pecl install nsq

    or:

    1. sudo phpize
    2. ./configure 
    3. make  
    4. make install  

    add in your php.ini:

    extension = nsq.so;
----------------------------------------------------------------

    Tips: 

    if has the error "configure: error: Cannot find libevent headers" you should run :

    ./configure --with-php-config=/usr/local/php-7.2.12/bin/php-config --with-libevent-path=/usr/local/libevent-2.1.8-stable/ 


### Example for pub:

```php
// Normal publish 
$nsqdAddr = array(
    "127.0.0.1:4150",
    "127.0.0.1:4154"
);

$nsq = new Nsq();
$isTrue = $nsq->connectNsqd($nsqdAddr);

for($i = 0; $i < 10000; $i++){
    $nsq->publish("test", "nihao");
}
$nsq->closeNsqdConnection();

// Deferred publish 
//function : deferredPublish(string topic,string message, int millisecond); 
//millisecond default : [0 < millisecond < 3600000]

$deferred = new Nsq();
$isTrue = $deferred->connectNsqd($nsqdAddr);
for($i = 0; $i < 20; $i++){
    $deferred->deferredPublish("test", "message daly", 3000); 
}
$deferred->closeNsqdConnection();

```

### Example for sub:
```php
<?php 

//sub

$nsq_lookupd = new NsqLookupd("127.0.0.1:4161"); //the nsqlookupd http addr
$nsq = new Nsq();
$config = array(
    "topic" => "test",
    "channel" => "struggle",
    "rdy" => 2,                //optional , default 1
    "connect_num" => 1,        //optional , default 1   
    "retry_delay_time" => 5000,  //optional, default 0 , if run callback failed, after 5000 msec, message will be retried
    "auto_finish" => true, //default true
);

$nsq->subscribe($nsq_lookupd, $config, function($msg,$bev){ 

    echo $msg->payload."\n";
    echo $msg->attempts."\n";
    echo $msg->messageId."\n";
    echo $msg->timestamp."\n";


});

```
### Nsq Object

* `connectNsqd($nsqdAddrArr)` <br/>
  publish use, You can also use it for health check;

* `closeNsqdConnection()` <br/>
  close connecNsqd's socket

* `publish($topic,$msg)` <br/>

* `deferredPublish($topic,$msg,$msec)` <br/>

* `subscribe($nsq_lookupd,$config,$callback)` <br/>

* `conn_timeout = 100`<br />
  connection timeout for `connectNsqd()` in milliseconds

### Message Object

The following properties and methods are available on Message objects produced by a Reader
instance.

* `timestamp` <br/>
  Numeric timestamp for the Message provided by nsqd.
* `attempts` <br/>
  The number of attempts that have been made to process this message.
* `messageId` <br/>
  The opaque string id for the Message provided by nsqd.
* `payload` <br/>
  The message payload as a Buffer object.
* `finish($bev,$msg->messageId)` <br/>
  Finish the message as successful.
* `touch($bev,$msg->messageId)` <br/>
  Tell nsqd that you want extra time to process the message. It extends the
  soft timeout by the normal timeout amount.



### Tips :


1. `If you need some variable in callback ,you should use 'use' :` <br/>

```
$nsq->subscribe($nsq_lookupd, $config, function($msg,$bev) use ($you_variable){ 

    echo $msg->payload;

});
```

2. `Requeue/Retry --  if you whant to retry your message when callback have something wrong, just throw any Exception , example:
` <br/>

```
<?php 

$nsq->subscribe($nsq_lookupd, $config, function($msg){ 
    try{
        echo $msg->payload . " " . "attempts:".$msg->attempts."\n";
        //do something
    }catch(Exception $e){

        if($msg->attempts < 3){
            //the message will be retried after you configure retry_delay_time
            throw new Exception("");
        }else{
            echo $e->getMessage();
            return;
        }
    }

});

```

3. `If you want to increase your message timeout and heartbeats time ,Two steps are needed: ` <br/>
```
    #1 when nsqd startup you should set command line option:

    nsqd --lookupd-tcp-address=127.0.0.1:4160 --max-heartbeat-interval=1m30s --msg-timeout=10m30s


    #2 And , you should use identify config. For details, see the identify example file.
    

```

4. `If your have strong consuming ability ,you can add you rdy num and connect num` <br/>


5. `If your execution time is more than 1 minute, you should use 'touch()' function ` <br/>
    
5. `Do not support calling publish or deferredPublish in subscribe function, please use nsqd's HTTP interface.` <br/>
    

Changes
-------
* **3.3.0**
  * add the process management
  * When the child process exits abnormally, it will pull up a new child process
  * When the master process exits, all child processes also exit
* **3.2.0**
  * Fix The error message was not reported
  * Fix pub error when ip or url too long
* **3.1.0**
  * Fix memmory wrong
  * Fix subscribe  wrong 
* **3.0**
  * Fix libevent more than 4096 bytes are truncated
  * add the identify command,can use be set or increase heartbeats time and msg-timeout
* **2.4.0**
  * Fix pub bug
  * Fix sub coredump 
  * Fix touch bug
  * Add the waite,  when topic has no message
* **2.3.1**
  * Support the domain host of pub
  * Fix pub coredump 
* **2.3.0**
  * Optimized memory usage,  Guarantee stability of resident memory 
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

