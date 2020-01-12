/*
 * nghttp2 - HTTP/2 C Library
 *
 * Copyright (c) 2014 Tatsuhiro Tsujikawa
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
// We wrote this code based on the original code which has the
// following license:
//
// main.cpp
// ~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

//#include <syslog.h>

#include <iostream>
#include <string>
#include <vector>
#include "config.h"

#include <boost/algorithm/string.hpp>

#include <nghttp2/asio_http2_server.h>

#include "json11.git/json11.hpp"

#ifdef bcm2385_found
#include "BCM2835.h"
#else
class Mockup {
  std::vector<bool> lights;
  std::vector<unsigned> pwm;
public:
  Mockup(int size, int pwm_size): lights(size, false), pwm(pwm_size, 50) {}
  bool switch_channel(int channel, bool value) {
    lights[channel]=value;
    return value;
  }
  bool get_channel(int channel) {
    return lights[channel];
  }
  unsigned size() const { return lights.size(); }
  unsigned get_pwm(int channel) const { return pwm[channel]; }
  unsigned set_pwm(int channel, unsigned p) { pwm[channel]=p; return p; }
  unsigned pwm_size() const { return pwm.size(); }
};
#endif

using namespace nghttp2::asio_http2;
using namespace nghttp2::asio_http2::server;

int main(int argc, char *argv[]) {
  //setlogmask (LOG_UPTO (LOG_INFO));
  //setlogmask (LOG_UPTO (LOG_DEBUG));
  //openlog ("pwm", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
  try {
    #ifdef bcm2385_found
    #if 0 // pingu
    BCM2835 backend { { 17, 27 }, { 18 } };
    backend.set_inverted(true);
    #else // luci
    BCM2835 backend { { 24, 23, 22, 17 }, { 18 } };
    backend.set_inverted(false);
    #endif
    backend.setup();
    #else
    Mockup backend(2, 1);
    #endif
    // Check command line arguments.
    if (argc < 4) {
      std::cerr
          << "Usage: lightsrv <address> <port> <threads> [<private-key-file> "
          << "<cert-file>]\n";
      return 1;
    }

    boost::system::error_code ec;

    std::string addr = argv[1];
    std::string port = argv[2];
    std::size_t num_threads = std::stoi(argv[3]);

    http2 server;

    server.num_threads(num_threads);

    #if 0
    server.handle("/", [](const request &req, const response &res) {
      (void)req;
      std::cerr << "DEBUG: in / handler" << std::endl;
      res.write_head(200, {{"foo", {"bar", false}}});
      res.end("hello, world\n");
    });
    #endif

    server.handle("/v1/switch/", [&backend](const request &req, const response &res) {
      std::cerr << "DEBUG: in /v1/switch/ handler" << std::endl;

      std::string path=req.uri().path;
      std::vector<std::string> paths;
      boost::split(paths, path, boost::is_any_of("/"));
      int channel=std::stoi(paths.back());

      if(req.method() == "PUT") {
        std::ostringstream *ostr=new std::ostringstream();
        req.on_data([&res, ostr, channel, &backend](const uint8_t *data, std::size_t len) {
          if(len>0) {
            ostr->write((const char *)data, len);
            return;
          }
          std::cerr << "DEBUG: received PUT request: " << ostr->str() << std::endl;

          // convert to json
          std::string err;
          std::string raw_body = ostr->str();
          delete ostr;

          json11::Json body = json11::Json::parse(raw_body, err);
          if(err.empty()) {
            bool value = body["on"].bool_value();
            backend.switch_channel(channel, value);
            res.write_head(200, {
              {"content-type", {"application/json", false}},
              {"Access-Control-Allow-Origin", {"*", false}}
            });
            json11::Json r = json11::Json::object {
              {
                "error", json11::Json::object {
                  { "code", 0 }
                }
              },
              { "request", json11::Json::object { { "on", value } } },
              {
                "response", json11::Json::object {
                  { "on", backend.get_channel(channel) }
                }
              }
            };
            std::cerr << "DEBUG: returning response: " << r.dump() << std::endl;
            res.end(r.dump());
          }
          else {
            std::cerr << "DEBUG: parse error on parsing json body: " << raw_body << std::endl;
            std::cerr << "DEBUG: partial json result: " << body.dump() << std::endl;
            std::cerr << "DEBUG: json parse error string: " << err << std::endl;
            json11::Json r = json11::Json::object {
              {
                "error", json11::Json::object {
                  { "code", 1 },
                  { "category", "json parse error" },
                  { "message", err }
                }
              }
            };
            std::cerr << "DEBUG: returning response: " << r.dump() << std::endl;
            res.end(r.dump());
          }
        });
      }
      else if(req.method() == "GET") {
        std::cerr << "DEBUG: received GET request." << std::endl;
        res.write_head(200, {{"content-type", {"application/json", false}}});
        json11::Json r = json11::Json::object {
          {
            "error", json11::Json::object {
              { "code", 0 }
            }
          },
          {
            "response", json11::Json::object {
              { "on", backend.get_channel(channel) }
            }
          }
        };
        std::cerr << "DEBUG: returning response: " << r.dump() << std::endl;
        res.end(r.dump());
      }
      else if(req.method() == "OPTIONS") {
        std::cerr << "DEBUG: received OPTIONS request." << std::endl;
        res.write_head(204, {
          {"content-length", {"0", false}},
          {"allow", {"OPTIONS,GET,PUT", false}},
          {"Access-Control-Allow-Origin", {"*", false}},
          {"Access-Control-Allow-Methods", {"GET,PUT,OPTIONS", false}},
          {"Access-Control-Allow-Headers", {"*", false}}
        });
        res.end();
      }      
      else {
        std::cerr << "DEBUG: unsupported request method for switch: " << req.method() << ", returning 400 Bad request" << std::endl;
        res.write_head(400);
        res.end("Bad request\n");
      }
    });

    server.handle("/v1/pwm/", [&backend](const request &req, const response &res) {
      std::cerr << "DEBUG: in /v1/pwm/ handler" << std::endl;

      std::string path=req.uri().path;
      std::vector<std::string> paths;
      boost::split(paths, path, boost::is_any_of("/"));
      int channel=std::stoi(paths.back());

      if(req.method() == "PUT") {
        std::ostringstream *ostr=new std::ostringstream();
        req.on_data([&res, ostr, channel, &backend](const uint8_t *data, std::size_t len) {
          if(len>0) {
            ostr->write((const char *)data, len);
            return;
          }
          std::cerr << "DEBUG: received PUT request: " << ostr->str() << std::endl;

          // convert to json
          std::string err;
          std::string raw_body = ostr->str();
          delete ostr;

          json11::Json body = json11::Json::parse(raw_body, err);
          if(err.empty()) {
            int value = body["value"].int_value();
            backend.set_pwm(channel, value);
            res.write_head(200, {
              {"content-type", {"application/json", false}},
              {"Access-Control-Allow-Origin", {"*", false}}
            });
            json11::Json r = json11::Json::object {
              {
                "error", json11::Json::object {
                  { "code", 0 }
                }
              },
              { "request", json11::Json::object { { "value", value } } },
              {
                "response", json11::Json::object {
                  { "value", (int)backend.get_pwm(channel) }
                }
              }
            };
            std::cerr << "DEBUG: returning response: " << r.dump() << std::endl;
            res.end(r.dump());
          }
          else {
            std::cerr << "DEBUG: parse error on parsing json body: " << raw_body << std::endl;
            std::cerr << "DEBUG: partial json result: " << body.dump() << std::endl;
            std::cerr << "DEBUG: json parse error string: " << err << std::endl;
            json11::Json r = json11::Json::object {
              {
                "error", json11::Json::object {
                  { "code", 1 },
                  { "category", "json parse error" },
                  { "message", err }
                }
              }
            };
            std::cerr << "DEBUG: returning response: " << r.dump() << std::endl;
            res.end(r.dump());
          }
        });
      }
      else if(req.method() == "GET") {
        std::cerr << "DEBUG: received GET request." << std::endl;
        res.write_head(200, {{"content-type", {"application/json", false}}});
        json11::Json r = json11::Json::object {
          {
            "error", json11::Json::object {
              { "code", 0 }
            }
          },
          {
            "response", json11::Json::object {
              { "value", (int)backend.get_pwm(channel) }
            }
          }
        };
        std::cerr << "DEBUG: returning response: " << r.dump() << std::endl;
        res.end(r.dump());
      }
      else if(req.method() == "OPTIONS") {
        std::cerr << "DEBUG: received OPTIONS request." << std::endl;
        res.write_head(204, {
          {"content-length", {"0", false}},
          {"allow", {"OPTIONS,GET,PUT", false}},
          {"Access-Control-Allow-Origin", {"*", false}},
          {"Access-Control-Allow-Methods", {"GET,PUT,OPTIONS", false}},
          {"Access-Control-Allow-Headers", {"*", false}}
        });
        res.end();
      }      
      else {
        std::cerr << "DEBUG: unsupported request method for pwm: " << req.method() << ", returning 400 Bad request" << std::endl;
        res.write_head(400);
        res.end("Bad request\n");
      }
    });

    server.handle("/v1/list", [&backend](const request &req, const response &res) {
      std::cerr << "DEBUG: in /v1/list handler" << std::endl;

      if(req.method() == "GET") {
        std::cerr << "DEBUG: received GET request." << std::endl;
        res.write_head(200, {
          {"content-type", {"application/json", false}},
          {"Access-Control-Allow-Origin", {"*", false}}
        });
        json11::Json::array switches;
        for(unsigned i=0; i<backend.size(); i++) switches.push_back(backend.get_channel(i));
        json11::Json::array pwms;
        for(unsigned i=0; i<backend.pwm_size(); i++) pwms.push_back((int)backend.get_pwm(i));
        json11::Json r = json11::Json::object {
          {
            "error", json11::Json::object {
              { "code", 0 }
            }
          },
          {
            "response", json11::Json::object {
              { "switches", switches },
              { "pwms", pwms },
            }
          }
        };
        std::cerr << "DEBUG: returning response: " << r.dump() << std::endl;
        res.end(r.dump());
      }
      else if(req.method() == "OPTIONS") {
        std::cerr << "DEBUG: received OPTIONS request." << std::endl;
        res.write_head(204, {
          {"content-length", {"0", false}},
          {"allow", {"OPTIONS,GET,PUT", false}},
          {"Access-Control-Allow-Origin", {"*", false}},
          {"Access-Control-Allow-Methods", {"GET,OPTIONS", false}},
          {"Access-Control-Allow-Headers", {"*", false}}
        });
        res.end();
      }
      else {
        std::cerr << "DEBUG: unsupported request method for list: " << req.method() << ", returning 400 Bad request" << std::endl;
        res.write_head(400);
        res.end("Bad request\n");
      }

    });

    server.handle("/", [&backend](const request &req, const response &res) {
      std::cerr << "DEBUG: in / handler" << std::endl;

      if(req.method() == "OPTIONS") {
        std::cerr << "DEBUG: received OPTIONS request." << std::endl;
        res.write_head(204, {
          {"content-length", {"0", false}},
          {"allow", {"OPTIONS,GET,PUT", false}},
          {"Access-Control-Allow-Origin", {"*", false}},
          {"Access-Control-Allow-Methods", {"GET,PUT,OPTIONS", false}},
          {"Access-Control-Allow-Headers", {"*", false}}
        });
        res.end();
      }
      else {
        std::cerr << "DEBUG: unsupported request method for /: " << req.method() << ", returning 400 Bad request" << std::endl;
        res.write_head(400);
        res.end("Bad request\n");
      }

    });

#if 0
    server.handle("/secret/", [](const request &req, const response &res) {
      (void)req;
      res.write_head(200);
      res.end("under construction!\n");
    });
    server.handle("/push", [](const request &req, const response &res) {
      (void)req;
      boost::system::error_code ec;
      auto push = res.push(ec, "GET", "/push/1");
      if (!ec) {
        push->write_head(200);
        push->end("server push FTW!\n");
      }

      res.write_head(200);
      res.end("you'll receive server push!\n");
    });
    server.handle("/delay", [](const request &req, const response &res) {
      (void)req;
      res.write_head(200);

      auto timer = std::make_shared<boost::asio::deadline_timer>(
          res.io_service(), boost::posix_time::seconds(3));
      auto closed = std::make_shared<bool>();

      res.on_close([timer, closed](uint32_t error_code) {
        (void)error_code;
        timer->cancel();
        *closed = true;
      });

      timer->async_wait([&res, closed](const boost::system::error_code &ec) {
        if (ec || *closed) {
          return;
        }

        res.end("finally!\n");
      });
    });
    server.handle("/trailer", [](const request &req, const response &res) {
      (void)req;
      // send trailer part.
      res.write_head(200, {{"trailers", {"digest", false}}});

      std::string body = "nghttp2 FTW!\n";
      auto left = std::make_shared<size_t>(body.size());

      res.end([&res, body, left](uint8_t *dst, std::size_t len,
                                 uint32_t *data_flags) {
        auto n = std::min(len, *left);
        std::copy_n(body.c_str() + (body.size() - *left), n, dst);
        *left -= n;
        if (*left == 0) {
          *data_flags |=
              NGHTTP2_DATA_FLAG_EOF | NGHTTP2_DATA_FLAG_NO_END_STREAM;
          // RFC 3230 Instance Digests in HTTP.  The digest value is
          // SHA-256 message digest of body.
          res.write_trailer(
              {{"digest",
                {"SHA-256=qqXqskW7F3ueBSvmZRCiSwl2ym4HRO0M/pvQCBlSDis=", false}}});
        }
        return n;
      });
    });
    #endif

    if (argc >= 6) {
      boost::asio::ssl::context tls(boost::asio::ssl::context::sslv23);
      tls.use_private_key_file(argv[4], boost::asio::ssl::context::pem);
      tls.use_certificate_chain_file(argv[5]);

      configure_tls_context_easy(ec, tls);

      if (server.listen_and_serve(ec, tls, addr, port)) {
        std::cerr << "error: " << ec.message() << std::endl;
      }
    } else {
      if (server.listen_and_serve(ec, addr, port)) {
        std::cerr << "error: " << ec.message() << std::endl;
      }
    }
  } catch (std::exception &e) {
    std::cerr << "exception: " << e.what() << "\n";
  }

  //closelog();
  return 0;
}
