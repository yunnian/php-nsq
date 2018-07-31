FROM php:alpine AS builder

ENV PHP_NSQ_VERSION=3.1.0

RUN apk add libevent-dev autoconf alpine-sdk
RUN wget "https://github.com/yunnian/php-nsq/archive/v${PHP_NSQ_VERSION}.tar.gz"
RUN tar xvzf "v${PHP_NSQ_VERSION}.tar.gz" 
WORKDIR "/php-nsq-${PHP_NSQ_VERSION}"
RUN phpize
RUN ./configure
RUN make 
RUN make install

FROM php:alpine

COPY --from=builder /usr/local/lib/php/extensions/no-debug-non-zts-20170718/nsq.so /usr/local/lib/php/extensions/no-debug-non-zts-20170718/nsq.so
RUN apk add --no-cache libevent && \
    echo "extension=nsq.so" > /usr/local/etc/php/conf.d/docker-php-ext-nsq.ini