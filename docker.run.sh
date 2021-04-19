#!/bin/bash

# build the project
make clean server

# build the docker image
docker rm server-relay
docker rmi server-relay
docker build -t server-relay .

# run it
docker run --name server-relay -it server-relay