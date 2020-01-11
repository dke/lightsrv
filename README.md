# lightsrv.git

Server copied from `nghttp2.git/examples/asio-sv.cc`

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
