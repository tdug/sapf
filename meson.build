project(
  'sapf',
  ['cpp', 'objcpp'],
  meson_version : '>=1.1'
)

cpp = meson.get_compiler('cpp')
deps = []
cpp_args = ['-std=c++17']
link_args = []

# libdispatch
cpp_args += ['-fblocks']
link_args += ['-lBlocksRuntime', '-ldispatch']

# libedit
deps += dependency('libedit', required: true)

if get_option('asan')
  cpp_args += ['-fsanitize=address', '-O1', '-fno-omit-frame-pointer', '-g']
  link_args += ['-fsanitize=address']
endif

if get_option('accelerate')
  add_project_arguments('-DSAPF_ACCELERATE', language: 'cpp')
else
  deps += dependency('fftw3', required: true, version: '>=3')
endif

if get_option('apple_lock')
  add_project_arguments('-DSAPF_APPLE_LOCK', language: 'cpp')
endif

if get_option('audiotoolbox')
  add_project_arguments('-DSAPF_AUDIOTOOLBOX', language: 'cpp')
else
  deps += dependency('rtaudio', required: true, version: '>=6')
endif

if get_option('carbon')
  add_project_arguments('-DSAPF_CARBON', language: 'cpp')
endif

if get_option('cocoa')
  add_project_arguments('-DSAPF_COCOA', language: 'cpp')
endif

if get_option('corefoundation')
  add_project_arguments('-DSAPF_COREFOUNDATION', language: 'cpp')
endif

if get_option('coremidi')
  add_project_arguments('-DSAPF_COREMIDI', language: 'cpp')
endif

if get_option('mach_time')
  add_project_arguments('-DSAPF_MACH_TIME', language: 'cpp')
endif

sources = [
  'src/CoreOps.cpp',
  'src/DelayUGens.cpp',
  'src/dsp.cpp',
  'src/elapsedTime.cpp',
  'src/ErrorCodes.cpp',
  'src/FilterUGens.cpp',
  'src/main.cpp',
  'src/makeImage.mm',
  'src/MathFuns.cpp',
  'src/MathOps.cpp',
  'src/Midi.cpp',
  'src/MultichannelExpansion.cpp',
  'src/Object.cpp',
  'src/Opcode.cpp',
  'src/OscilUGens.cpp',
  'src/Parser.cpp',
  'src/Play.cpp',
  'src/primes.cpp',
  'src/RCObj.cpp',
  'src/RandomOps.cpp',
  'src/SetOps.cpp',
  'src/SoundFiles.cpp',
  'src/Spectrogram.cpp',
  'src/StreamOps.cpp',
  'src/symbol.cpp',
  'src/Types.cpp',
  'src/UGen.cpp',
  'src/VM.cpp',
]

executable(
  'sapf',
  sources,
  include_directories: [include_directories('include')],
  dependencies: deps,
  cpp_args : cpp_args,
  link_args : link_args
)
