#!/bin/bash

# build the project
make clean server

# build the docker image
docker rmi server-relay
docker build -t server-relay .

# run it
docker rm server-relay
docker run --name server-relay -it server-relay