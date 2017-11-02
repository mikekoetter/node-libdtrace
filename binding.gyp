{
  'targets': [
    {
      'target_name': 'libdtrace',
      'cflags_cc': ['-fexceptions'],
      'ldflags': ['-ldtrace'],
      'sources': [ 
        'libdtrace.cc'
      ],
      'conditions' : [
          ['OS=="mac" or OS=="solaris"', {
	      	"include_dirs" : ["<!(node -e \"require('nan')\")"],
 	      }
	  ], 
	  ['OS=="freebsd"',
                            { 'include_dirs': [
                                  '/usr/src/sys/cddl/compat/opensolaris',
                                  '/usr/src/sys/cddl/contrib/opensolaris/uts/common/',
				  '/usr/src/cddl/contrib/opensolaris/lib/libdtrace/common/',
                                 '<!(node -e "require(\'nan\')")'
                              ]
                            }]
      ],
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
