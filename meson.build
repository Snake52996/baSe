project('baSe', 'c',
  version : '0.1',
  default_options : ['warning_level=3', 'c_std=c2x'])

# These arguments are only used to build the shared library
# not the executables that use the library.
lib_args = ['']

shlib = library('baSe', 'bitio.c', 'heap.c', 'keyvalue_pair.c', 'list.c', 'RAII.c', 'vector.c', 'hashtable.c',
  install : true,
  c_args : lib_args,
)

# Make this library usable from the system's
# package manager.
install_headers('bitio.h', 'heap.h', 'keyvalue_pair.h', 'list.h', 'RAII.h', 'vector.h', 'hashtable.h', subdir : 'baSe')
