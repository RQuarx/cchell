project('cchell', 'cpp',
        version:       '1.0.0-alpha',
        meson_version: '>=1.4.0',
        license:       'GPL-3.0-or-later',
        license_files:   [ 'COPYING' ],
        default_options: [ 'cpp_std=c++23',
                           'warning_level=3' ])


cchell_include    = include_directories('include')
cchell_build_args = [ '-DPROJECT_NAME="@0@"'   .format(meson.project_name()),
                      '-DPROJECT_VERSION="@0@"'.format(meson.project_version()),
                      '-D_GNU_SOURCE' ]

cchell_deps = [ dependency('lyra', fallback: ['Lyra', 'lyra_dep']) ]


subdir('src') # provides cchell_source
executable(meson.project_name(), cchell_source,
           include_directories: cchell_include,
           cpp_args:            cchell_build_args,
           dependencies:        cchell_deps)