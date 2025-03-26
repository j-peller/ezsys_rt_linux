# Use an ARM64 Ubuntu base image
FROM arm64v8/ubuntu:24.04 AS build

# Set environment variables to non-interactive (useful for automated builds)
ENV DEBIAN_FRONTEND=noninteractive

# Update and install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    libgpiod-dev

# Set working directory
WORKDIR /app

# Copy your source code into the container
COPY . .

# Configure and build your project
#RUN cmake -DCMAKE_BUILD_TYPE=Debug . && make
RUN cmake -DCMAKE_BUILD_TYPE=${BUILD_TYPE} . && make

# Stage 2: Export the final binary
FROM scratch AS export-stage
# Copy the final binary from the build stage.
# Adjust the path and binary name accordingly.
COPY --from=build /app/RPISignal /RPISignal