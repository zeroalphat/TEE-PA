FROM debian:buster-slim as builder

RUN apt-get update \
    && apt-get install -y git make gcc\
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app/
RUN git clone https://github.com/linaro-swg/soc_term \
    && cd soc_term \
    && make

FROM debian:buster-slim as runner

COPY --from=builder /app/soc_term/soc_term /app/soc_term

ENTRYPOINT ["/app/soc_term"]
CMD ["54321"]
