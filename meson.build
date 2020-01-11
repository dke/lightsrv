project('lightsrv', 'cpp',
        default_options : ['warning_level=3', 'cpp_std=c++14'])

thread_dep = dependency('threads')
boost_dep = dependency('boost', modules : ['system'])

nghttp2_dep = dependency('libnghttp2_asio', version : '>=1.40')

openssl_dep = dependency('openssl')

incdir = include_directories('json11.git')

cc = meson.get_compiler('cpp')
lib_bcm2835 = cc.find_library('bcm2835', required : false)

conf_data = configuration_data()
conf_data.set('bcm2385_found', lib_bcm2835.found())
configure_file(input : 'config.h.in', output : 'config.h', configuration : conf_data)

sources = ['main.cc', 'json11.git/json11.cpp']

if lib_bcm2835.found()
    sources += ['BCM2835.cc']
endif

exe = executable('lightsrv', sources,
        dependencies : [thread_dep, boost_dep, nghttp2_dep, openssl_dep, lib_bcm2835])
