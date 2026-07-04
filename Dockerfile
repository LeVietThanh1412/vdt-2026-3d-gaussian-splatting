# Multi-stage Dockerfile for 3DGS AR Pipeline

# Stage 1: Build the C++ CLI tool
FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    curl \
    zip \
    unzip \
    tar \
    pkg-config \
    libgl1-mesa-dev \
    libegl1-mesa-dev \
    libxrandr-dev \
    libxinerama-dev \
    libxcursor-dev \
    libxi-dev \
    python3 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY . .

RUN chmod +x scripts/*.sh && \
    ./scripts/fetch_third_party.sh && \
    ./scripts/build.sh

FROM python:3.9-slim AS web_demo

WORKDIR /app

COPY web_demo /app/web_demo
COPY data /app/data

EXPOSE 8000

CMD ["python3", "web_demo/serve.py"]
