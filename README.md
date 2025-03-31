# Build Instructions

To build the project for Raspberry Pi (aarch64) using Docker, you can use the following command:

```bash
docker buildx build --platform linux/arm64 --build-arg BUILD_TYPE=Release --output type=local,dest=./build
```

This will create a build output in the `./build` directory, targeting the Raspberry Pi architecture.
