# http_srv 
Shares files via HTTP protocol.\
Waits for clients connection at the specified address.

### how to build:
1. `meson setup build -Db_sanitize=address,undefined`
2. `meson compile -C build`
3. `ninja -C build clang-format`

### how to run:
`http_srv -d <dir_path> -a <ip address>:<port>`

### hot to test:
1. run server\
`http_srv -d /home/alex/Downloads -a 127.0.0.1:8081`\
The server expects that requested file passed in `file` custom header

2. run client(for ex. curl)\
`curl -v 127.0.0.1:8081 -H "file:19-threads-homework.pdf" --output /home/alex/Desktop/threads_hw.pdf`

client output example:
```
% Total    % Received % Xferd  Average Speed   Time    Time     Time  Current
                                 Dload  Upload   Total   Spent    Left  Speed
  0     0    0     0    0     0      0      0 --:--:-- --:--:-- --:--:--     0*   Trying 127.0.0.1:8081...
* Connected to 127.0.0.1 (127.0.0.1) port 8081 (#0)
> GET / HTTP/1.1
> Host: 127.0.0.1:8081
> User-Agent: curl/7.87.0
> Accept: */*
> file:19-threads-homework.pdf
> 
* Mark bundle as not supporting multiuse
< HTTP/1.1 200 OK
< Server: FileSrv
< Content-Length: 51924
< Connection: close
< 
{ [51924 bytes data]
100 51924  100 51924    0     0  47.6M      0 --:--:-- --:--:-- --:--:-- 49.5M
* Closing connection 0
```

server output example:
```
start listening on 127.0.0.1:8081...
new client accepted: 127.0.0.1:43794
client request:
GET / HTTP/1.1
Host: 127.0.0.1:8081
User-Agent: curl/7.87.0
Accept: */*
file:19-threads-homework.pdf


requested file: "/home/alex/Downloads/19-threads-homework.pdf"
file size: 51924 bytes
file has been sent
client disconnected
```
