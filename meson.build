project('epie', 'c',
  version : '0.1.4',
  default_options : ['c_std=c99', 'warning_level=3'])

sources = []
subdir('src')

add_project_arguments('-Wno-unused-function', '-Isrc/toml/', '-Isrc/termios/', language : 'c')
executable('epie', sources, install : true)
