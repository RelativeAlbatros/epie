project('epie', 'c',
  version : '0.1.4',
  default_options : ['c_std=c99', 'warning_level=3'])

add_project_arguments('-Wunused-function', language : 'c')
executable('epie', ['epie.c', 'lib/toml.c', 'lib/termiWin.c'], install : true)
