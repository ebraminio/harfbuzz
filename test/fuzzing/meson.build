tests = [
  'hb-shape-fuzzer.cc',
  'hb-subset-fuzzer.cc',
  'hb-set-fuzzer.cc',
  'hb-draw-fuzzer.cc',
]

foreach file_name : tests
  test_name = file_name.split('.')[0]

  sources = [file_name]
  fuzzer_ldflags = []
  extra_cpp_args = []

  if get_option('fuzzer_ldflags') == ''
    sources += 'main.cc'
  else
    fuzzer_ldflags += get_option('fuzzer_ldflags').split()
  endif

  if get_option('failing_alloc')
    extra_cpp_args += '-DHB_IS_FAILING_ALLOC'
  endif

  exe = executable(test_name, sources,
    cpp_args: cpp_args + extra_cpp_args,
    include_directories: [incconfig, incsrc],
    link_args: fuzzer_ldflags,
    link_with: [libharfbuzz, libharfbuzz_subset],
    install: false,
  )
  set_variable('@0@_exe'.format(test_name.underscorify()), exe)
endforeach

env = environment()
env.set('srcdir', meson.current_source_dir())

test('shape_fuzzer', find_program('run-shape-fuzzer-tests.py'),
  args: [
    hb_shape_fuzzer_exe,
  ],
  timeout: 60,
  depends: [hb_shape_fuzzer_exe, libharfbuzz, libharfbuzz_subset],
  workdir: join_paths(meson.current_build_dir(), '..', '..'),
  env: env,
  suite: ['fuzzing', 'slow'],
)

test('subset_fuzzer', find_program('run-subset-fuzzer-tests.py'),
  args: [
    hb_subset_fuzzer_exe,
  ],
  # as the tests are ran concurrently let's raise acceptable time here
  # ideally better to break and let meson handles them in parallel
  timeout: 300,
  workdir: join_paths(meson.current_build_dir(), '..', '..'),
  env: env,
  suite: ['fuzzing', 'slow'],
)

test('draw_fuzzer', find_program('run-draw-fuzzer-tests.py'),
  args: [
    hb_draw_fuzzer_exe,
  ],
  workdir: join_paths(meson.current_build_dir(), '..', '..'),
  env: env,
  suite: ['fuzzing'],
)
