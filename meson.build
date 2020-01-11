project('lightsrv', 'cpp',
        default_options : ['warning_level=3', 'cpp_std=c++14'])

thread_dep = dependency('threads')
boost_dep = dependency('boost', modules : ['system'])

nghttp2_dep = dependency('libnghttp2_asio', version : '>=1.40')

openssl_dep = dependency('openssl')

incdir = include_directories('json11.git')

exe = executable('lightsrv', ['main.cc', 'json11.git/json11.cpp'], dependencies : [thread_dep, boost_dep, nghttp2_dep, openssl_dep])
