project('power-profiles-daemon', [ 'c' ],
        version: '0.23',
        license: 'GPLv3+',
        default_options: [
          'buildtype=debugoptimized',
          'warning_level=1',
          'c_std=c99',
        ],
        meson_version: '>= 0.59.0')

cc = meson.get_compiler('c')

common_cflags = cc.get_supported_arguments([
    '-fgnu89-inline',
    '-Wundef',
    '-Wunused',
    '-Wstrict-prototypes',
    '-Werror-implicit-function-declaration',
    '-Wno-pointer-sign',
    '-Wshadow',
    '-Wno-sign-compare',
    '-Wno-cast-function-type',
    '-Wno-unused-parameter',
    '-Wno-missing-field-initializers',
    '-Wno-type-limits',
])

prefix = get_option('prefix')
libexecdir = prefix / get_option('libexecdir')
bindir = get_option('bindir')
dbusconfdir = get_option('datadir') / 'dbus-1' / 'system.d'
dbusservicedir = get_option('datadir') / 'dbus-1' / 'system-services'

systemd_system_unit_dir = get_option('systemdsystemunitdir')
if systemd_system_unit_dir == 'auto'
    systemd_dep = dependency('systemd')
    systemd_system_unit_dir = systemd_dep.get_variable('systemdsystemunitdir')
endif
glib_dep = dependency('glib-2.0')
gio_unix_dep = dependency('gio-unix-2.0')
gio_dep = dependency('gio-2.0')
gudev_dep = dependency('gudev-1.0', version: '>= 234')
upower_dep = dependency('upower-glib')
polkit_gobject_dep = dependency('polkit-gobject-1', version: '>= 0.99')
polkit_policy_directory = polkit_gobject_dep.get_variable('policydir')

python3_required_modules = []
gi_required_modules = {}

powerprofilesctl_required_gi_modules = {
  'GLib': '2.0',
  'Gio': '2.0',
}

gnome = import('gnome')

add_global_arguments('-D_GNU_SOURCE=1', language: 'c')
add_global_arguments(common_cflags, language: 'c')

pylint = find_program('pylint-3', 'pylint3', 'pylint', required: get_option('pylint'))
if pylint.found()
    nomalloc = environment({'MALLOC_PERTURB_': '0'})
    pylint_flags = ['-d', 'C0116', '-d', 'C0114', '-d', 'W0707', '-d', 'W0706' ]
endif
xmllint = find_program('xmllint', required: false)

argparse_manpage = find_program('argparse-manpage', required: get_option('manpage'))
if argparse_manpage.found()
  gi_required_modules += powerprofilesctl_required_gi_modules
endif

bus_names = {
  'org.freedesktop.UPower.PowerProfiles': '/org/freedesktop/UPower/PowerProfiles',
  'net.hadess.PowerProfiles': '/net/hadess/PowerProfiles',
}

address_sanitizer = get_option('b_sanitize') == 'address' or \
  get_option('b_sanitize') == 'address,undefined' or \
  get_option('b_sanitize') == 'leak'

python = import('python')
python3 = python.find_installation('python3')

script = 'import importlib.util; import sys; exit(1) if importlib.util.find_spec(\''+ 'shtab' +'\') is None else exit(0)'
if run_command(python3, '-c', script, check: false).returncode() == 0
  has_shtab = true
else
  has_shtab = false
endif

bashcomp = dependency('bash-completion', required: get_option('bashcomp').disable_auto_if(not has_shtab))
zshcomp = get_option('zshcomp') != ''

if bashcomp.found() or zshcomp
  python3_required_modules += 'shtab'
  gi_required_modules += powerprofilesctl_required_gi_modules
endif

if get_option('tests')
  python3_required_modules += [
    'dbusmock',
  ]
  gi_required_modules += powerprofilesctl_required_gi_modules
  gi_required_modules += {
    'UMockdev': '1.0',
  }
endif

# Python 3 required modules
if gi_required_modules.keys().length() > 0
  python3_required_modules += 'gi'
endif

foreach p : python3_required_modules
  # Source: https://docs.python.org/3/library/importlib.html#checking-if-a-module-can-be-imported
  script = 'import importlib.util; import sys; exit(1) if importlib.util.find_spec(\''+ p +'\') is None else exit(0)'
  if run_command(python3, '-c', script, check: false).returncode() == 0
    message('Python3 module \'@0@\' found: YES'.format(p))
  else
    error('Python3 module \'@0@\' required but not found'.format(p))
  endif
endforeach

foreach module, version : gi_required_modules
  script = 'import gi; gi.require_version("@0@", "@1@")'.format(module, version)
  if run_command(python3, '-c', script, check: false).returncode() == 0
    message('Python3 module \'@0@\' found: YES @1@'.format(module, version))
  else
    error('''GObject Introspection module '@0@' version @1@ required but not found'''.format(
      module, version))
  endif
endforeach

subdir('src')
subdir('data')

if get_option('gtk_doc')
  # Make COPYING available in the build root for docs
  configure_file(
    input: 'COPYING',
    output: 'COPYING',
    copy: true,
  )
  subdir('docs')
endif

if get_option('tests')
  subdir('tests')
endif

meson.add_dist_script(
  find_program('check-news.sh').full_path(),
  '@0@'.format(meson.project_version())
)

summary({
  'tests': get_option('tests'),
  'bash-completion': bashcomp,
  'zsh-completion': zshcomp,
  'manpages': argparse_manpage.found(),
  'python linting': pylint.found(),
  'gtk_doc': get_option('gtk_doc'),
})
