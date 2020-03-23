cmake_minimum_required(VERSION 3.7)
project(php-nsq)
set(CMAKE_CXX_STANDARD 11)
set(SOURCE_FILES
        php_nsq.h
        nsq.c
        nsq_exception.h
        nsq_exception.c
        nsq_lookupd.c
        nsq_lookupd.h
        command.c
        command.h
        common.c
        command.h
        sub.c
        sub.h
        pub.c
        pub.h
        message.h
        message.c

        )

add_executable(php-nsq ${SOURCE_FILES})

#定义php源码路径，这里根据自己的真实路径来更改
set(PHP_SOURCE /data/php-src)
#引入php需要的扩展源码，这里也是根据自己需要的来更改
include_directories(${PHP_SOURCE}/main)
include_directories(${PHP_SOURCE}/Zend)
include_directories(${PHP_SOURCE}/sapi)
include_directories(${PHP_SOURCE}/pear)
include_directories(${PHP_SOURCE}/TSRM)
include_directories(${PHP_SOURCE})

add_custom_target(makefile COMMAND sudo phpize && ./configure &&   make
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})