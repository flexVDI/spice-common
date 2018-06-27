#
# Build tests
#
tests = ['test-logging', 'test-region']
tests_deps = [spice_common_dep]

if spice_common_generate_code == 'all'
  tests += ['test-overflow']
  tests_deps += [spice_common_client_dep, spice_common_server_dep]
endif

foreach t : tests
  name = t.underscorify()
  exe = executable(name, '@0@.c'.format(t),
                   dependencies : tests_deps,
                   install : false)
  test(name, exe)
endforeach

#
# test_marshallers
#
test_proto = files('test-marshallers.proto')

test_marshallers_sources = ['test-marshallers.c', 'test-marshallers.h']

targets = [
    ['test_marshallers', test_proto, 'generated_test_marshallers.c', ['--generate-marshallers', '--server', '--include', 'test-marshallers.h', '@INPUT@', '@OUTPUT@']],
    ['test_marshallers_h', test_proto, 'generated_test_marshallers.h', ['--generate-marshallers', '--server', '--include', 'test-marshallers.h', '-H', '@INPUT@', '@OUTPUT@']],
    ['test_demarshallers', test_proto, 'generated_test_demarshallers.c', ['--generate-demarshallers', '--client', '--include', 'test-marshallers.h', '@INPUT@', '@OUTPUT@']],
]

foreach t : targets
  cmd = [python, spice_codegen] + t[3]
  test_marshallers_sources += custom_target(t[0], input: t[1], output : t[2], command: cmd, depend_files : spice_codegen_files)
endforeach

test('test_marshallers',
     executable('test_marshallers', test_marshallers_sources,
                dependencies : spice_common_dep,
                install : false))

#
# test_quic
#
gdk_pixbuf_dep = dependency('gdk-pixbuf-2.0', version : '>= 2.26', required : false)
if gdk_pixbuf_dep.found()
  test('test_quic',
       executable('test_quic', 'test-quic.c',
                  dependencies : [spice_common_dep, gdk_pixbuf_dep],
                  install : false), timeout : 120)
endif
