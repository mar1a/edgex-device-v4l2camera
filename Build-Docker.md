## Build Docker image:

You may also need to add your user to the `docker` group:
```
sudo usermod -a -G docker $USER
newgrp docker
```

Build a docker image by using the following command
```
cd edgex-device-v4l2camera
docker build . -t edgex-device-v4l2camera -f ./scripts/Dockerfile.alpine-3.9

```
This command will build the edgex-device-v4l2camera release image.