FROM alpine:3 as build
RUN apk add gcc musl-dev make
RUN mkdir /src
ADD . /src
RUN cd /src && make static && strip wait-for-it.static

FROM scratch
COPY --from=build /src/wait-for-it.static /wait-for-it
ENTRYPOINT ["/wait-for-it"]
