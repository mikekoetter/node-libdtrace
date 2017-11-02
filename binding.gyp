{
  'targets': [
    {
      'target_name': 'libdtrace',
      'cflags_cc': ['-fexceptions'],
      'ldflags': ['-ldtrace'],
      'sources': [ 
        'libdtrace.cc'
      ], 
      "include_dirs" : ["<!(node -e \"require('nan')\")"],
      'libraries': ['-ldtrace'],
      'xcode_settings': {
          'OTHER_CPLUSPLUSFLAGS': [
              '-fexceptions',
              '-Wunused-variable',
          ],
      }
    },
  ]
}
