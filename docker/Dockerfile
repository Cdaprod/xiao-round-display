FROM python:3.12-slim

ARG PLATFORMIO_VERSION=6.1.18

ENV PIP_DISABLE_PIP_VERSION_CHECK=1 \
    PYTHONUNBUFFERED=1

RUN apt-get update \
    && apt-get install --yes --no-install-recommends ca-certificates git \
    && rm -rf /var/lib/apt/lists/* \
    && python -m pip install --no-cache-dir "platformio==${PLATFORMIO_VERSION}"

COPY tools/pio-dev.py /usr/local/bin/pio-dev

WORKDIR /workspace

CMD ["pio-dev"]
