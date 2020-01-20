# lightsrv.git

Server copied from `nghttp2.git/examples/asio-sv.cc`

## Dependencies

* Boost
* nghttp2 with asio bindings (you'll probably need to build that yourself, or are there packages providing this?)
* C library for Broadcom BCM 2835 as used in Raspberry Pi : https://www.airspayce.com/mikem/bcm2835/

### BCM 2835

After building and installing, create and install a `pkg-config` file manually. Assuming you installed to `/usr/local`, the file looks like:

```
prefix=/usr/local
exec_prefix=${prefix}
libdir=${exec_prefix}/lib
includedir=${prefix}/include

Name: libbcm2835
Description: Raspberry Pi BCM2835 GPIO library
URL: https://www.airspayce.com/mikem/bcm2835/
Version: 1.36
Libs: -L${libdir} -lbcm2835
Cflags: -I${includedir}
```

Install that one to `${prefix}/lib/pkgconfig/libbcm2835.pc`.

## Build it

After checkout:

```
meson build
cd build/
ninja
```

## Prereqs

### Create SSL cert

```
# "-nodes" is for no passphrase:
openssl req -x509 -newkey rsa:4096 -keyout key.pem -out cert.pem -days 365 -nodes
```

### Start the daemon

```
# in build/ dir
./lightsrv 0.0.0.0 8888 1 ../key.pem ../cert.pem
```

If you use the "real" bcm backend (not the mockup one), that'll probably need root privileges, unless you find out how to do it without (and if so, then please tell me).

### Test it

```
$ curl -k --http2 "https://d10-dev.lan:8888/v1/switch/0"; echo
{"response": {"on": false}, "error": {"code": 0}}
$ curl -k --http2 -X PUT -H "Content-Type: application/json" -d '{"on":true}' "https://d10-dev.lan:8888/v1/switch/0"; echo
{"response": {"on": true}, "error": {"code": 0}, "request": {"on": true}}
$ curl -k --http2 "https://d10-dev.lan:8888/v1/switch/0"; echo
{"error": {"code": 0}, "response": {"on": false}}
$ curl -k --http2 -X PUT -H "Content-Type: application/json" -d '{"on":false}' "https://d10-dev.lan:8888/v1/switch/0"; echo
{"error": {"code": 0}, "request": {"on": false}, "response": {"on": false}}
```

### Installation

There is a systemd file. You can install it in `/etc/systemd/system` and enable and start it in the usual way.

This service defintion expects the binary to live in /usr/local/sbin as lightsrv and load the `key.pem` and `cert.pem` files from `/usr/local/etc/lightsrv/`. Furthermore it expects to load `index.html` from its CWD /usr/local/etc/lightsrv.

### HTML/Javascript client

See `index.html`.

The file is delived by the service via the / or /index.html path. You can also copy it statically to a client and change the url in the appropriate location in the file.

In order to work, you need to make the browser accept the self-signed cert. Easiest to do so is to access some sever URL in the browser directly and follow the browsers questions.

## TODO

* one open/close call per auto run
* command line arguments for initial autorun, or setup only, or switch/* pwm stuff
* configurable auto interval
* config file
* backend parameters also by config file / arguments
* api level checking to not allow manual controls if auto is active
* api: input sanitization / checking
* [DONE] find out how to daemon / systemd
* [DONE] delivering the HTML file from the lightsrv program itself
