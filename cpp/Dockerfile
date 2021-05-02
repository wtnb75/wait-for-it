FROM alpine:3 as build
RUN apk add gcc g++ musl-dev make gflags-dev boost-dev
RUN mkdir /src
ADD . /src
RUN cd /src && make && strip wait-for-it

FROM alpine:3
RUN apk add --no-cache gflags boost libstdc++
COPY --from=build /src/wait-for-it /wait-for-it
ENTRYPOINT ["/wait-for-it"]
