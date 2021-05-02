FROM golang:1-alpine as dev
RUN apk add make
RUN mkdir s
ADD . /s
RUN cd /s && CGO_ENABLED=0 make

FROM scratch
COPY --from=dev /s/wait-for-it /wait-for-it
ENTRYPOINT ["/wait-for-it"]
