#!/bin/bash

# build the docker image
docker stop server-relay
docker rm server-relay
docker rmi server-relay
docker build -t server-relay .

# run it
docker run -p 60000:60000 --network=rehoboam --name server-relay -d server-relay