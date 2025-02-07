project('sapf', ['cpp', 'objcpp'])

cpp = meson.get_compiler('cpp')

if cpp.has_header('Accelerate/Accelerate.h')
  add_project_arguments('-DSAPF_ACCELERATE', language: 'cpp')
endif

if cpp.has_header('os/lock.h')
  add_project_arguments('-DSAPF_APPLE_LOCK', language: 'cpp')
endif

if cpp.has_header('AudioToolbox/AudioToolbox.h ')
  add_project_arguments('-DSAPF_AUDIOTOOLBOX', language: 'cpp')
endif

if cpp.has_header('Carbon/Carbon.h')
  add_project_arguments('-DSAPF_CARBON', language: 'cpp')
endif

if cpp.has_header('Cocoa/Cocoa.h')
  add_project_arguments('-DSAPF_COCOA', language: 'cpp')
endif

if cpp.has_header('CoreFoundation/CoreFoundation.h')
  add_project_arguments('-DSAPF_COREFOUNDATION', language: 'cpp')
endif

if cpp.has_header('CoreMidi/CoreMidi.h')
  add_project_arguments('-DSAPF_COREMIDI', language: 'cpp')
endif

if cpp.has_header('mach/mach_time.h')
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
  cpp_args : '-fblocks',
  link_args : ['-lfftw3','-lBlocksRuntime','-ldispatch','-ledit']
)
