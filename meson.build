project(
    'MesonRpc',
    'c','cpp',
    version : '0.1.0',
    meson_version: '>= 0.61.0',
    default_options: [
        'warning_level=2',
        'buildtype=release',
        'cpp_std=c++20'])
protobuf_dep = dependency('protobuf', required: true)
threads = dependency('threads')

includes_dir = include_directories('./src/include')
# cpp             = meson.get_compiler('cpp')
#libcups         = dependency('cups')
# libhv         = dependency('hv')
libhv = declare_dependency(
  include_directories: include_directories('/usr/local/include/hv'),
  link_args: ['-L/usr/local/lib', '-lhv']
)

client_sources = run_command('find', './src/client', '-type', 'f', '-name', '*[.c,.cpp,.h,.hpp,.cc]', check : false).stdout().strip().split('\n')
server_sources = run_command('find', './src/server', '-type', 'f', '-name', '*[.c,.cpp,.h,.hpp,.cc]', check : false).stdout().strip().split('\n')
common_sources = run_command('find', './src/common', '-type', 'f', '-name', '*[.c,.cpp,.h,.hpp,.cc]', check : false).stdout().strip().split('\n')

executable(
    'client',
    [common_sources,client_sources],
    c_args : ['-DCLIENT'],
    cpp_args : ['-DCLIENT'],
    include_directories: includes_dir,
    dependencies : [
        libhv,protobuf_dep,threads
    ],
    install : false)

executable(
    'server',
    [common_sources,server_sources],
    c_args : ['-DSERVER'],
    cpp_args : ['-DSERVER'],
    include_directories: includes_dir,
    dependencies : [
        libhv,protobuf_dep,threads
    ],
    install : false)