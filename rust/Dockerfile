FROM rust:1-alpine as dev
RUN mkdir src
ADD . /src
RUN cd /src && cargo build --release
RUN cd /src && strip target/release/wait-for-it

FROM scratch
COPY --from=dev /src/target/release/wait-for-it /wait-for-it
ENTRYPOINT ["/wait-for-it"]
