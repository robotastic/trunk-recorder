
## Build
From the root directory of trunk-recorder, run:
`docker build -t tr-arch -f utils/Dockerfile.arch-latest.dev .`

*On an M1 based Mac:*
`docker build -t tr-arch -f utils/Dockerfile.arch-latest.dev --platform=linux/amd64 .`


## Run
`docker run --platform=linux/amd64 -v ${PWD}:/src -it tr-arch  /bin/bash` 

`docker run -v ${PWD}:/src -it trunkrecorder:dev /bin/bash`  