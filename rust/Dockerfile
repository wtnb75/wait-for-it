FROM rust:1-alpine as dev
RUN apk add --no-cache git
RUN mkdir src
ADD . /src
RUN cd /src && cargo build --release --config net.git-fetch-with-cli=true
RUN cd /src && strip target/release/wait-for-it

FROM scratch
COPY --from=dev /src/target/release/wait-for-it /wait-for-it
ENTRYPOINT ["/wait-for-it"]
