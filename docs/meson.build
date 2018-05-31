if get_option('manual')
  asciidoc = find_program('asciidoc', required : false)
  if asciidoc.found()
    custom_target('spice_protocol.html',
                  input : files('spice_protocol.txt'),
                  output : 'spice_protocol.html',
                  build_by_default : true,
                  command : [asciidoc, '-n', '-a', 'icons', '-a', 'toc', '-o', '@OUTPUT@', '@INPUT@'])
  endif
endif
