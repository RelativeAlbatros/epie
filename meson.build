project('kilo', 'c',
  version : '0.1.1',
  default_options : ['c_std=c99', 'warning_level=3'])

thread_dep = dependency('threads')
executable('kilo', ['kilo.c', 'toml.c'], dependencies: thread_dep, install : true)
