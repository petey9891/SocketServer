#!/bin/bash

# build the docker image
docker rm server-relay
docker rmi server-relay
docker build -t server-relay .

# run it
docker run -p 60000:60000 --name server-relay -it server-relay