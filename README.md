# lightsrv.git

Server copied from `nghttp2.git/examples/asio-sv.cc`

## Dependencies

* Boost
* nghttp2 with asio bindings (you'll probably need to build that yourself, or are there packages providing this?)
* C library for Broadcom BCM 2835 as used in Raspberry Pi : https://www.airspayce.com/mikem/bcm2835/

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

### HTML/Javascript client

See `lightui.html`.

Change the `$url` to point to the actual service url.

In order to work, you need to make the browser accept the self-signed cert. Easiest to do so is to access some sever URL in the browser directly and follow the browsers questions.