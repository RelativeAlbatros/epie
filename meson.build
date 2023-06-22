project('kilo', 'c',
  version : '0.1.0',
  default_options : ['c_std=c99', 'warning_level=3'])

thread_dep = dependency('threads')
executable('kilo', 'kilo.c', 'logger/logger.c', dependencies: thread_dep, install : true)
