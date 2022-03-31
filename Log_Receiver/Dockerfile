FROM python:3.10-buster as builder

WORKDIR /app/

COPY  src/requirements.txt /app/
RUN pip3 install --no-cache-dir -r requirements.txt

FROM python:3.10-slim-buster as runner

COPY --from=builder /usr/local/lib/python3.10/site-packages /usr/local/lib/python3.10/site-packages

RUN apt-get update \
    && apt-get install -y --no-install-recommends libffi-dev=3.2.1-9 \
    && apt-get -y clean \
    && rm -rf /var/lib/apt/lists/*

COPY src/* /app/

EXPOSE 8080
CMD ["python", "/app/main.py"]