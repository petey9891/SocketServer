FROM alpine:3.13 as build

RUN apk add --no-cache --virtual .build-deps make g++ openssl-dev linux-headers

WORKDIR /server
COPY . .

RUN make

RUN apk del .build-deps make g++

CMD [ "bin/ServerRelay" ]

