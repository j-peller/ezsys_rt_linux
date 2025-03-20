# Build Instructions

To build the project for Yocto on Raspberry Pi (aarch64) using Docker, you can use the following command:

```bash
sudo docker build -t rpisignal-crossbuild --output type=local,dest=./build . -f Dockerfile.yocto
```

or use this command to build it for normal Ubuntu on Rapsberry Pi:
```bash
sudo docker buildx build --platform linux/arm64 --build-arg BUILD_TYPE=Release --output type=local,dest=./build . -f Dockerfile
```

This will create a build output in the `./build` directory, targeting the Raspberry Pi architecture.