#
# project definition
#
project('spice-common', 'c',
         meson_version : '>= 0.45.0',
         license : 'LGPLv2.1')

if not meson.is_subproject()
  warning('This project is only intended to be used as a subproject!')
endif

# some global vars
spice_common_global_cflags = [#'-std=c99', # fails compiling bitops.h
                              '-DHAVE_CONFIG_H',
                              '-DG_LOG_DOMAIN="Spice"',
                              '-Wall',
                              '-Wextra',
                              '-Werror',
                              '-Wno-unused-parameter']

if get_option('alignment-checks')
  spice_common_global_cflags += ['-DSPICE_DEBUG_ALIGNMENT']
endif

spice_common_deps = []
spice_common_include = include_directories('.')

spice_proto = files('spice.proto')
spice1_proto = files('spice1.proto')
spice_codegen = files('spice_codegen.py')
spice_codegen_files = [spice_codegen]

compiler = meson.get_compiler('c')
spice_common_config_data = configuration_data()
if get_option('extra-checks')
  spice_common_config_data.set('ENABLE_EXTRA_CHECKS', '1')
endif

spice_common_generate_code = get_option('generate-code')
spice_common_generate_client_code = spice_common_generate_code == 'all' or spice_common_generate_code == 'client'
spice_common_generate_server_code = spice_common_generate_code == 'all' or spice_common_generate_code == 'server'

#
# check for system headers
#
headers = ['alloca.h',
           'arpa/inet.h',
           'dlfcn.h',
           'inttypes.h',
           'malloc.h',
           'memory.h',
           'netinet/in.h',
           'stddef.h',
           'stdint.h',
           'stdlib.h',
           'strings.h',
           'string.h',
           'sys/socket.h',
           'sys/stat.h',
           'sys/types.h',
           'unistd.h',
           'vfork.h']

foreach header : headers
  if compiler.has_header(header)
    spice_common_config_data.set('HAVE_@0@'.format(header.underscorify().to_upper()), '1')
  endif
endforeach

#
# check for system functions
#
functions = ['alloca',
             'dup2',
             'floor',
             'fork',
             'memmove',
             'memset',
             'vfork']

foreach func : functions
  if compiler.has_function(func)
    spice_common_config_data.set('HAVE_@0@'.format(func.to_upper()), '1')
  endif
endforeach

# check for hypot function
#
# Include math.h header to avoid problems with builtins.
# In some systems the function is in libm.
if not compiler.has_function('hypot', prefix : '#include <math.h>')
  libm = compiler.find_library('m', required : false)
  if compiler.has_function('hypot', prefix : '#include <math.h>', dependencies : libm)
    spice_common_deps += libm
  endif
endif

#
# check for mandatory dependencies
#
glib_version = '2.38'
glib_version_info = '>= @0@'.format(glib_version)

deps = [['spice-protocol', '>= 0.12.12'],
        ['glib-2.0', glib_version_info],
        ['gio-2.0', glib_version_info],
        ['gthread-2.0', glib_version_info],
        ['pixman-1', '>= 0.17.7'],
        ['openssl', '>= 1.0.0']]

foreach dep : deps
  spice_common_deps += dependency(dep[0], version : dep[1])
endforeach

#
# Non-mandatory/optional dependencies
#
# Check deps which are optional but enabled by default. This foreach block only
# checks the option, and adds the package to the deps list, while the real check
# for the dependency is done in the foreach block below.
optional_deps = [
                    ['celt051', '>= 0.5.1.1', 'HAVE_CELT051'],
                    ['opus', '>= 0.9.14', 'HAVE_OPUS'],
                ]
foreach dep : optional_deps
  option_value = get_option(dep[0])
  if option_value != 'false'
    d = dependency(dep[0], required: (option_value == 'true'), version : dep[1])
    if d.found()
      spice_common_deps += d
      spice_common_config_data.set(dep[2], '1')
    endif
  endif
endforeach

# Python
if get_option('python-checks')
  dependency('python3')
  py_module = import('python3')
  python = py_module.find_python()
  foreach module : ['six', 'pyparsing']
    cmd = run_command(python, '-m', module)
    if cmd.returncode() != 0
      error('Python module @0@ not found'.format(module))
    endif
  endforeach
endif

# smartcard check
smartcard_dep = dependency('libcacard', required : false, version : '>= 2.5.1')
if smartcard_dep.found()
  spice_common_deps += smartcard_dep
  spice_common_config_data.set('USE_SMARTCARD', '1')
else
  smartcard012_dep = dependency('libcacard', required : false, version : '>= 0.1.2')
  if smartcard012_dep.found()
    spice_common_deps += smartcard012_dep
    spice_common_config_data.set('USE_SMARTCARD_012', '1')
  endif
endif

#
# global C defines
#
glib_major_minor = glib_version.split('.')
glib_encoded_version = 'GLIB_VERSION_@0@_@1@'.format(glib_major_minor[0], glib_major_minor[1])
spice_common_global_cflags += ['-DGLIB_VERSION_MIN_REQUIRED=@0@'.format(glib_encoded_version),
                               '-DGLIB_VERSION_MAX_ALLOWED=@0@'.format(glib_encoded_version)]

foreach arg : spice_common_global_cflags
  add_project_arguments(arg, language : 'c')
endforeach

#
# Subdirectories
#
subdir('python_modules')
subdir('common')
subdir('tests')
subdir('docs')

#
# write config.h
#
configure_file(output : 'config.h',
               install : false,
               configuration : spice_common_config_data)
