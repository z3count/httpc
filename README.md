# httpc — a tiny HTTP client in C

A minimal yet robust **HTTP/1.1 GET client** written in C.  
It builds a GET request, sends it over POSIX sockets (IPv4/IPv6), parses the response, and prints the result.  
Comes with a few **unit tests**.

> Goal: provide a **clear, self-contained, and extensible** learning base for writing a simple HTTP client without third-party libraries.

---

## ✨ Features

- TCP connection (IPv4/IPv6) with timeouts  
- HTTP/1.1 **GET** requests with custom headers  
- Parses **status line**, **headers**, and **body**  
- Basic **unit tests** included  
- No global state, short readable functions  

> Exception to the no-3rd-party-lib assertion: TLS support is now implemented, using the libopenssl.  Writing our own TLS support for GET requests is most certainly a bad idea.


---

## Usage

You can specify a few options via the CLI.
```bash
bin/httpc -h
```

For the full list of available options:
```bash
 --host <hostname> --port <port number> --path <well, the path>
 ```
 
 If you want to display some specific informations about the HTTP reply:
 ```bash
 --get-code --get-headers --get-body
 ```
 
 You can also specify the headers used for the request:
 ```bash
 --http-header "Foo: bar-a-vin" --http-header "Bar: whatever"
 ```

> We assume the body is human-readable, so brace yourself and don't be surprised if your terminal is ruined when you download a random binary file.


---

## ⚙️ Requirements

- POSIX system (Linux, maybe another POSIX-like OS, but not tested)  
- C compiler (tested with gcc 4.8.*) and `make`  
- You must have installed the `libopenssl-dev` package to support TLS connections__

---

## 🧱 Build

```bash
make
```
This builds:

the final binary:
```bash
bin/httpc
```

To clean:
```bash
make clean
```

To run the unit tests:
```bash
$ make check
cc -g -ggdb -O0 -D_GNU_SOURCE -Iinclude -I/usr/local/include -Wall -Wextra -Werror -std=c99 -o objs/cli.o -c src/cli.c
...
cc -g -ggdb -O0 -D_GNU_SOURCE -Iinclude -I/usr/local/include -Wall -Wextra -Werror -std=c99 -o objs/utest.o -c src/utest.c
cc -o bin/httpc -g -ggdb -O0 -D_GNU_SOURCE -Iinclude -I/usr/local/include -Wall -Wextra -Werror -std=c99 objs/cli.o objs/http.o objs/http_headers.o objs/http_parse.o objs/http_reply.o objs/http_request.o objs/logger.o objs/main.o objs/network.o objs/strutil.o objs/usage.o objs/utest.o 
bin/httpc -t
[src/../tests/strutil_utest.c:trim_utest:65] Successes: 7, Failures: 0
[src/cli.c:cli_parse_header:83] missing header colon-separator: foo
[src/cli.c:cli_parse_header:83] missing header colon-separator: 
[src/../tests/cli_utest.c:cli_parse_header_utest:101] Successes: 3, Failures: 0
[src/../tests/http_parse_utest.c:http_parse_code_utest:143] Successes: 7, Failures: 0
[src/../tests/http_parse_utest.c:http_parse_headers_utest:70] Successes: 7, Failures: 0
[src/http_parse.c:http_parse_body:176] must provide a non-NULL body
[src/http_parse.c:http_parse_body:181] unable to locate the CRLF CRLF header/body delimiter
[src/http_parse.c:http_parse_body:181] unable to locate the CRLF CRLF header/body delimiter
[src/../tests/http_parse_utest.c:http_parse_body_utest:225] Successes: 6, Failures: 0

All unit tests passed!
```

## 🛠️ Possible extensions (will never happen)

- Add POST/PUT/PATCH with request body
- Handle redirects
- Support Keep-Alive / connection reuse
- Integrate HTTP/2 via ALPN + TLS
- Add proxy / CONNECT support

## 📄 License

WTFPL (https://www.wtfpl.net/)

## 👨‍💻 Note

This project is intentionally educational and minimal — each function focuses on clarity, correctness, and testability.

Don't contact me.
