FROM alpine:3.13 as build

RUN apk add --no-cache --virtual .build-deps .build-deps make g++ openssl-dev linux-headers

WORKDIR /server
COPY . .

RUN make

CMD [ "bin/ServerRelay" ]

