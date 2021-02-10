# lightsrv.git

Server copied from `nghttp2.git/examples/asio-sv.cc`

## Dependencies

* Boost
* nghttp2 with asio bindings (you'll probably need to build that yourself, or are there packages providing this?) **and our custom patch applied**
* C library for Broadcom BCM 2835 as used in Raspberry Pi : https://www.airspayce.com/mikem/bcm2835/

### nghttp2

Installed some prereqs like for lowdoh, i.e.

```
sudo apt install build-essential libldns-dev meson ninja-build pkg-config libboost-all-dev libasio-dev libevent-dev libev-dev libssl-dev zlib1g-dev libxml2-dev libjansson-dev libjemalloc-dev pkg-config
```

As of time of writing, latest master (some 1.44.0-DEV, 40679cf638541729752470900004c5acfdb247d1) was working. But we still need our custom patch.


```
git clone https://github.com/nghttp2/nghttp2.git nghttp2.upstream.git
cd nghttp2.upstream.git
patch -p1 < ../lightsrv/patch-for-lightsrv.patch
```

Building according to https://nghttp2.org/documentation/package_README.html#building-from-git but we add the `--enable-asio-lib` switch.

```
git submodule update --init
autoreconf -i
automake
autoconf
./configure --enable-asio-lib
make
sudo make install
```


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

In your build directory:

```
ninja install
```

This installs the binary, the config file, the selfsigned key / cert, the `index.html` file, and the systemd service file.

You can invoke with with a `DESTDIR` environment variable set to install it into a staging area for transferring to other hosts.

### HTML/Javascript client

See `index.html`.

The file is delived by the service via the / or /index.html path. You can also copy it statically to a client and change the url in the appropriate location in the file.

In order to work, you need to make the browser accept the self-signed cert. Easiest to do so is to access some sever URL in the browser directly and follow the browsers questions.

## TODO

* command line arguments for initial autorun, or setup only, or switch/* pwm stuff
* api level checking to not allow manual controls if auto is active
* api: input sanitization / checking
* [DONE] one open/close call per auto run
* [DONE] configurable auto interval
* [DONE] config file
* [DONE] backend parameters also by config file / arguments
* [DONE] find out how to daemon / systemd
* [DONE] delivering the HTML file from the lightsrv program itself
