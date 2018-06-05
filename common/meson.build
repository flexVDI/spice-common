#
# libspice-common
#
spice_common_sources = [
  'backtrace.c',
  'backtrace.h',
  'canvas_utils.c',
  'canvas_utils.h',
  'draw.h',
  'lines.c',
  'lines.h',
  'log.c',
  'log.h',
  'lz.c',
  'lz.h',
  'lz_common.h',
  'lz_config.h',
  'macros.h',
  'marshaller.c',
  'marshaller.h',
  'mem.c',
  'mem.h',
  'messages.h',
  'pixman_utils.c',
  'pixman_utils.h',
  'quic.c',
  'quic.h',
  'quic_config.h',
  'rect.h',
  'region.c',
  'region.h',
  'ring.h',
  'rop3.c',
  'rop3.h',
  'snd_codec.c',
  'snd_codec.h',
  'spice_common.h',
  'verify.h'
]

spice_common_lib = static_library('spice-common', spice_common_sources,
                                  install : false,
                                  include_directories : spice_common_include,
                                  dependencies : spice_common_deps)

spice_common_dep = declare_dependency(link_with : spice_common_lib,
                                      include_directories : spice_common_include,
                                      dependencies : spice_common_deps)

#
# libspice-common-client
#
if spice_common_generate_client_code
  targets = [
    ['client_demarshallers', spice_proto, 'generated_client_demarshallers.c', ['--generate-demarshallers', '--client', '--include', 'common/messages.h', '@INPUT@', '@OUTPUT@']],
    ['client_demarshallers1', spice1_proto, 'generated_client_demarshallers1.c', ['--generate-demarshallers', '--client', '--include', 'common/messages.h', '--prefix', '1', '--ptrsize', '8', '@INPUT@', '@OUTPUT@']],
    ['client_marshalers', spice_proto, 'generated_client_marshallers.c', ['--generate-marshallers', '-P', '--client', '--include', 'client_marshallers.h', '@INPUT@', '@OUTPUT@']],
    ['client_marshallers1', spice1_proto, 'generated_client_marshallers1.c', ['--generate-marshallers', '-P', '--client',  '--include', 'common/messages.h', '--include', 'client_marshallers.h', '--prefix', '1', '--ptrsize', '8', '@INPUT@', '@OUTPUT@']],
    ['client_marshallers_h', spice_proto, 'generated_client_marshallers.h', ['--generate-marshallers', '-P', '--client', '--include', 'common/messages.h', '-H', '@INPUT@', '@OUTPUT@']],
  ]

  spice_common_client_sources = [
    'client_demarshallers.h',
    'client_marshallers.h',
    'ssl_verify.c',
    'ssl_verify.h',
  ]

  foreach t : targets
    cmd = [python, spice_codegen] + t[3]
    target = custom_target(t[0], input : t[1], output : t[2], install : false, command : cmd, depend_files : spice_codegen_files)
    spice_common_client_sources += target
  endforeach

  spice_common_client_lib = static_library('spice-common-client', spice_common_client_sources,
                                           install : false,
                                           dependencies : spice_common_dep)

  spice_common_client_dep = declare_dependency(sources : target,
                                               link_with : spice_common_client_lib,
                                               dependencies : spice_common_dep)
endif

#
# libspice-common-server
#
if spice_common_generate_server_code
  structs_args = [
   '-M', 'String',
   '-M', 'Rect',
   '-M', 'Point',
   '-M', 'DisplayBase',
   '-M', 'Fill',
   '-M', 'Opaque',
   '-M', 'Copy',
   '-M', 'Blend',
   '-M', 'Blackness',
   '-M', 'Whiteness',
   '-M', 'Invers',
   '-M', 'Rop3',
   '-M', 'Stroke',
   '-M', 'Text',
   '-M', 'Transparent',
   '-M', 'AlphaBlend',
   '-M', 'Composite',
  ]

  targets = [
    ['server_demarshallers', spice_proto, 'generated_server_demarshallers.c', ['--generate-demarshallers', '--server', '--include', 'common/messages.h', '@INPUT@', '@OUTPUT@']],
    ['server_marshallers', spice_proto, 'generated_server_marshallers.c', ['--generate-marshallers', '--server'] + structs_args + ['--include', 'common/messages.h', '@INPUT@', '@OUTPUT@']],
    ['server_marshallers_h', spice_proto, 'generated_server_marshallers.h', ['--generate-marshallers', '--server'] + structs_args + ['--include', 'common/messages.h', '-H', '@INPUT@', '@OUTPUT@']],
  ]

  spice_common_server_sources = []

  foreach t : targets
    cmd = [python, spice_codegen] + t[3]
    target = custom_target(t[0], input : t[1], output : t[2], install : false, command : cmd, depend_files : spice_codegen_files)
    spice_common_server_sources += target
  endforeach

  spice_common_server_lib = static_library('spice-common-server', spice_common_server_sources,
                                           install : false,
                                           dependencies : spice_common_dep)

  spice_common_server_dep = declare_dependency(sources : target,
                                               link_with : spice_common_server_lib,
                                               dependencies : spice_common_dep)
endif
