project('lightsrv', 'cpp',
        default_options : ['warning_level=3', 'cpp_std=c++14'])


# just for getting the install dir for systemd system unit files
systemd_dep = dependency('systemd')

# supposedly some mesons have an option for that, but mine
# fails hard with an unrecoverable error, so I need to
# comment that out

#servicedir = get_option('systemd-system-conf-dir')
#if servicedir == ''
#  servicedir = systemd_dep.get_pkgconfig_variable('systemdsystemconfdir')
#endif
servicedir = systemd_dep.get_pkgconfig_variable('systemdsystemconfdir')
if servicedir == ''
  error('Couldn\'t determine systemd user unit service directory. You may want to disable systemd related stuff in meson.build and hardcode a install_dir for the service file.')
endif

thread_dep = dependency('threads')

boost_dep = dependency('boost', modules : ['system', 'program_options'])

nghttp2_dep = dependency('libnghttp2_asio', version : '>=1.40')

openssl_dep = dependency('openssl')

incdir = include_directories('json11.git')

bcm2835_dep = dependency('libbcm2835', required : false)

conf_data = configuration_data()
conf_data.set('bcm2385_found', bcm2835_dep.found())
configure_file(input : 'config.h.in', output : 'config.h', configuration : conf_data)

sources = ['main.cc', 'PeriodicTask.cc', 'BCM2835.cc', 'json11.git/json11.cpp']

exe = executable('lightsrv', sources,
        dependencies : [thread_dep, boost_dep, nghttp2_dep, openssl_dep, bcm2835_dep],
        install : true, install_dir : get_option('sbindir'))

install_data('lightsrv.service', install_dir : servicedir )
install_data('key.pem', install_dir: join_paths(get_option('sysconfdir'), 'lightsrv'))
install_data('cert.pem', install_dir: join_paths(get_option('sysconfdir'), 'lightsrv'))
install_data('index.html', install_dir: join_paths(get_option('sysconfdir'), 'lightsrv'))
install_data('lightsrv.conf', install_dir: join_paths(get_option('sysconfdir'), 'lightsrv'))
