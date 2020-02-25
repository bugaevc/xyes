project('xyes', 'c',
    version: '0.1.0',
    license: 'GPL3+',
    meson_version: '>= 0.44.0',
)

xlib = dependency('X11')
executable('xyes', 'xyes.c', dependencies: xlib, install: true)

install_man('xyes.1')
