project('lightsrv', 'cpp',
        default_options : ['warning_level=3', 'cpp_std=c++14'])

thread_dep = dependency('threads')
boost_dep = dependency('boost', modules : ['system'])

nghttp2_dep = dependency('libnghttp2_asio', version : '>=1.40')

openssl_dep = dependency('openssl')

incdir = include_directories('json11.git')

bcm2835_dep = dependency('libbcm2835', required : false)

conf_data = configuration_data()
conf_data.set('bcm2385_found', bcm2835_dep.found())
configure_file(input : 'config.h.in', output : 'config.h', configuration : conf_data)

sources = ['main.cc', 'json11.git/json11.cpp']

if bcm2835_dep.found()
    sources += ['BCM2835.cc']
endif

exe = executable('lightsrv', sources,
        dependencies : [thread_dep, boost_dep, nghttp2_dep, openssl_dep, bcm2835_dep])
