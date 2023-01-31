project('olcPGEX_DrawingTablet', 'cpp')

x11dep = dependency('x11')
pngdep = dependency('libpng')
gldep = dependency('gl')
xidep = dependency('xi')

executable('demo', 'src/main.cpp', dependencies: [x11dep, pngdep, gldep, xidep],
  override_options: ['cpp_std=c++17'])
