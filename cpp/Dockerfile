FROM alpine:3 as build
RUN apk add gcc g++ musl-dev make boost-dev
RUN mkdir /src
ADD . /src
RUN cd /src && make && strip wait-for-it

FROM alpine:3
RUN apk add --no-cache boost-program_options libstdc++
COPY --from=build /src/wait-for-it /wait-for-it
ENTRYPOINT ["/wait-for-it"]
