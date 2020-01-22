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

#include <syslog.h>

#include <ctime>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <chrono>

#include "config.h"

#include <boost/algorithm/string.hpp>
#include <boost/bind.hpp>
#include <boost/format.hpp>

#include <nghttp2/asio_http2_server.h>

#include "json11.git/json11.hpp"
#include "PeriodicTask.h"

#include "BCM2835.h"

using namespace nghttp2::asio_http2;
using namespace nghttp2::asio_http2::server;

int main(int argc, char *argv[]) {
  //setlogmask (LOG_UPTO (LOG_INFO));
  setlogmask (LOG_UPTO (LOG_DEBUG));
  openlog ("lightsrv", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);

  syslog (LOG_NOTICE, "Program started by User %d", getuid ());
  syslog (LOG_INFO, "A tree falls in a forest");
  syslog (LOG_DEBUG, "Another tree falls in a forest");

  try {
    #if 1 // pingu
    // 18: RPI_GPIO_P1_12
    // third argument: has_automode; default: false
    // fourth argument: inverted; default: false
    // fifth argument: debug; default: false
    BCM2835 backend { { 17, 27 }, { }, true, true };
    #else // luci
    BCM2835 backend { { 24, 23, 22, 17 }, { 18 }, true };
    #endif
    backend.setup();
    backend.set_auto(false);
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
      syslog(LOG_DEBUG, "in / handler");
      res.write_head(200, {{"foo", {"bar", false}}});
      res.end("hello, world\n");
    });
    #endif

    server.handle("/v1/switch/", [&backend](const request &req, const response &res) {
      syslog(LOG_DEBUG, "in /v1/switch/ handler");

      std::string path=req.uri().path;
      std::vector<std::string> paths;
      boost::split(paths, path, boost::is_any_of("/"));
      int channel=std::stoi(paths.back());
      syslog(LOG_INFO, "received %s %s request", req.method().c_str(), req.uri().path.c_str());

      if(req.method() == "PUT") {
        std::ostringstream *ostr=new std::ostringstream();
        req.on_data([&res, ostr, channel, &backend](const uint8_t *data, std::size_t len) {
          if(len>0) {
            ostr->write((const char *)data, len);
            return;
          }

          syslog(LOG_INFO, "PUT data: %s", ostr->str().c_str());
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
            syslog(LOG_DEBUG, "returning response: %s", r.dump().c_str());
            res.end(r.dump());
          }
          else {
            syslog(LOG_DEBUG, "parse error on json body: %s", raw_body.c_str());
            syslog(LOG_DEBUG, "partial json result: %s", body.dump().c_str());
            syslog(LOG_DEBUG, "json parse error string: %s", err.c_str());
            json11::Json r = json11::Json::object {
              {
                "error", json11::Json::object {
                  { "code", 1 },
                  { "category", "json parse error" },
                  { "message", err }
                }
              }
            };
            syslog(LOG_DEBUG, "returning response: %s", r.dump().c_str());
            res.end(r.dump());
          }
        });
      }
      else if(req.method() == "GET") {
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
        syslog(LOG_DEBUG, "returning response: %s", r.dump().c_str());
        res.end(r.dump());
      }
      else if(req.method() == "OPTIONS") {
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
        syslog(LOG_DEBUG, "unsupported request method for switch: %s, returning 400 Bad request", req.method().c_str());
        res.write_head(400);
        res.end("Bad request\n");
      }
    });

    server.handle("/v1/pwm/", [&backend](const request &req, const response &res) {
      syslog(LOG_DEBUG, "in /v1/pwm/ handler");

      std::string path=req.uri().path;
      std::vector<std::string> paths;
      boost::split(paths, path, boost::is_any_of("/"));
      int channel=std::stoi(paths.back());
      syslog(LOG_INFO, "received %s %s request", req.method().c_str(), req.uri().path.c_str());

      if(req.method() == "PUT") {
        std::ostringstream *ostr=new std::ostringstream();
        req.on_data([&res, ostr, channel, &backend](const uint8_t *data, std::size_t len) {
          if(len>0) {
            ostr->write((const char *)data, len);
            return;
          }
          syslog(LOG_DEBUG, "PUT data: %s", ostr->str().c_str());

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
            syslog(LOG_DEBUG, "returning response: %s", r.dump().c_str());
            res.end(r.dump());
          }
          else {
            syslog(LOG_DEBUG, "parse error on json body: %s", raw_body.c_str());
            syslog(LOG_DEBUG, "partial json result: %s", body.dump().c_str());
            syslog(LOG_DEBUG, "json parse error string: %s", err.c_str());
            json11::Json r = json11::Json::object {
              {
                "error", json11::Json::object {
                  { "code", 1 },
                  { "category", "json parse error" },
                  { "message", err }
                }
              }
            };
            syslog(LOG_DEBUG, "returning response: %s", r.dump().c_str());
            res.end(r.dump());
          }
        });
      }
      else if(req.method() == "GET") {
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
        syslog(LOG_DEBUG, "returning response: %s", r.dump().c_str());
        res.end(r.dump());
      }
      else if(req.method() == "OPTIONS") {
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
        syslog(LOG_DEBUG, "unsupported request method for pwm: %s, returning 400 Bad request", req.method().c_str());
        res.write_head(400);
        res.end("Bad request\n");
      }
    });

    server.handle("/v1/list", [&backend](const request &req, const response &res) {
      syslog(LOG_DEBUG, "in /v1/list handler");
      syslog(LOG_INFO, "received %s %s request", req.method().c_str(), req.uri().path.c_str());

      if(req.method() == "GET") {
        res.write_head(200, {
          {"content-type", {"application/json", false}},
          {"Access-Control-Allow-Origin", {"*", false}}
        });
        json11::Json::array switches;
        for(unsigned i=0; i<backend.size(); i++) switches.push_back(backend.get_channel(i));
        json11::Json::array pwms;
        for(unsigned i=0; i<backend.pwm_size(); i++) pwms.push_back((int)backend.get_pwm(i));
        auto autoo = json11::Json::object  {
          { "available", backend.has_autom() }
        };
        if(backend.has_autom()) {
          autoo["value"] = backend.get_auto();
        }
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
              { "auto", autoo }
            }
          }
        };
        syslog(LOG_DEBUG, "returning response: %s", r.dump().c_str());
        res.end(r.dump());
      }
      else if(req.method() == "OPTIONS") {
        res.write_head(204, {
          {"content-length", {"0", false}},
          {"allow", {"OPTIONS,GET", false}},
          {"Access-Control-Allow-Origin", {"*", false}},
          {"Access-Control-Allow-Methods", {"GET,OPTIONS", false}},
          {"Access-Control-Allow-Headers", {"*", false}}
        });
        res.end();
      }
      else {
        syslog(LOG_DEBUG, "unsupported request method for list: %s, returning 400 Bad request", req.method().c_str());
        res.write_head(400);
        res.end("Bad request\n");
      }

    });

    server.handle("/v1/auto", [&backend](const request &req, const response &res) {
      syslog(LOG_DEBUG, "in /v1/auto handler");
      syslog(LOG_INFO, "received %s %s request", req.method().c_str(), req.uri().path.c_str());

      if(req.method() == "PUT") {
        std::ostringstream *ostr=new std::ostringstream();
        req.on_data([&res, ostr, &backend](const uint8_t *data, std::size_t len) {
          if(len>0) {
            ostr->write((const char *)data, len);
            return;
          }
          syslog(LOG_DEBUG, "PUT data: %s", ostr->str().c_str());

          // convert to json
          std::string err;
          std::string raw_body = ostr->str();
          delete ostr;

          json11::Json body = json11::Json::parse(raw_body, err);
          if(err.empty()) {
            std::string value_str = body["on"].string_value();
            syslog(LOG_DEBUG, "auto: value_str=%s", value_str.c_str());
            bool value = body["on"].bool_value();
            syslog(LOG_DEBUG, "auto: value=%d", value);
            backend.set_auto(value);
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
                  { "value", (bool)backend.get_auto() }
                }
              }
            };
            syslog(LOG_DEBUG, "returning response: %s", r.dump().c_str());
            res.end(r.dump());
          }
          else {
            syslog(LOG_DEBUG, "parse error on json body: %s", raw_body.c_str());
            syslog(LOG_DEBUG, "partial json result: %s", body.dump().c_str());
            syslog(LOG_DEBUG, "json parse error string: %s", err.c_str());
            json11::Json r = json11::Json::object {
              {
                "error", json11::Json::object {
                  { "code", 1 },
                  { "category", "json parse error" },
                  { "message", err }
                }
              }
            };
            syslog(LOG_DEBUG, "returning response: %s", r.dump().c_str());
            res.end(r.dump());
          }
        });
      }
      else if(req.method() == "GET") {
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
              { "value", backend.get_auto() }
            }
          }
        };
        syslog(LOG_DEBUG, "returning response: %s", r.dump().c_str());
        res.end(r.dump());
      }
      else if(req.method() == "OPTIONS") {
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
        syslog(LOG_DEBUG, "unsupported request method for list: %s, returning 400 Bad request", req.method().c_str());
        res.write_head(400);
        res.end("Bad request\n");
      }

    });

server.handle("/", [&backend](const request &req, const response &res) {
      syslog(LOG_DEBUG, "in / handler");
      syslog(LOG_INFO, "received %s %s request", req.method().c_str(), req.uri().path.c_str());

      if(req.method() == "OPTIONS") {
        res.write_head(204, {
          {"content-length", {"0", false}},
          {"allow", {"OPTIONS,GET,PUT", false}},
          {"Access-Control-Allow-Origin", {"*", false}},
          {"Access-Control-Allow-Methods", {"GET,PUT,OPTIONS", false}},
          {"Access-Control-Allow-Headers", {"*", false}}
        });
        res.end();
      }

      else if(req.method() == "GET") {

        auto path = percent_decode(req.uri().path);
        if (path == "/") {
          path = "/index.html";
        }

        //if (!check_path(path)) {
        if (path != "/index.html") {
          res.write_head(404);
          res.end();
          return;
        }

        //path = docroot + path;
        auto fd = open("index.html", O_RDONLY);
        if (fd == -1) {
          res.write_head(404);
          res.end();
          return;
        }

        auto header = header_map();

        struct stat stbuf;
        if (stat(path.c_str(), &stbuf) == 0) {
          header.emplace("content-length",
                        header_value{std::to_string(stbuf.st_size), false});
          header.emplace("last-modified",
                        header_value{http_date(stbuf.st_mtime), false});
        }
        res.write_head(200, std::move(header));
        res.end(file_generator_from_fd(fd));

      }

      else {
        syslog(LOG_INFO, "unsupported request method for /: %s, returning 400 Bad request", req.method().c_str());
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

    std::shared_ptr<boost::asio::ssl::context> ptls(nullptr);
    if (argc >= 6) {
      ptls=std::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::sslv23);
      ptls->use_private_key_file(argv[4], boost::asio::ssl::context::pem);
      ptls->use_certificate_chain_file(argv[5]);
      configure_tls_context_easy(ec, *ptls);
    }

    server.reset();
    boost::asio::io_service &sv = server.io_service();
    std::shared_ptr<PeriodicTask> task(nullptr);
    if(backend.has_autom()) {
      syslog(LOG_INFO, "Installing automode handler with an interval of %d seconds", 5);
      task = std::make_shared<PeriodicTask>(sv, "Automode Handler", 5, [&backend](){ backend.autom(); }, true);
    }
    else {
      syslog(LOG_INFO, "Not installing automode handler since the backend does not support it");
    }
    if (server.no_reset_listen_and_serve(ec, ptls.get(), addr, port)) {
      std::cerr << "error: " << ec.message() << std::endl;
    }

  } catch (std::exception &e) {
    std::cerr << "exception: " << e.what() << "\n";
  }

  closelog();
  return 0;
}
