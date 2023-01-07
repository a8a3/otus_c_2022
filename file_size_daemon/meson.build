project(
    'fszd', ['c'],
    default_options : ['c_std=c11'],
    version: '0.0.1'
)

sources = ['src/main.c', 'src/config.c']
inc_dir = include_directories('include')
libyaml = subproject('libyaml')
yaml_dep = libyaml.get_variable('yaml_dep')

executable(
    meson.project_name(), 
    sources, 
    c_args: ['-Wall', '-Wextra', '-Wpedantic'],
    include_directories: inc_dir,
    dependencies: [yaml_dep]
)

# copy cfg file to build dir
cfg_file = join_paths(meson.source_root(), 'cfg/fszd.yaml')
build_dir = meson.build_root()
message('copying @0@ to @1@ ...'.format(cfg_file, build_dir))
run_command('cp', cfg_file, build_dir, capture: false, check: true)
