FROM alpine:3.13 as build

RUN apk add --no-cache --virtual .build-deps .build-deps make g++ openssl-dev linux-headers

WORKDIR /server
COPY . .

RUN make

FROM alpine:3.13
COPY --from=build /server/bin /server/bin
COPY --from=build /server/certs /server/certs

CMD [ "bin/ServerRelay" ]

