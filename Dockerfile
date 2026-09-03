FROM ubuntu:24.04

RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
        cmake \
        g++ \
        libgtest-dev \
        make \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . .
ENTRYPOINT ["bash", "/src/harness.sh"]
CMD ["build", "fast"]
