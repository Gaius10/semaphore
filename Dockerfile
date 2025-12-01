FROM gcc

WORKDIR /app
COPY . .
RUN make

CMD ["./main", "default"]
