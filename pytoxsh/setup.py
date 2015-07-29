from cx_Freeze import setup, Executable

# Dependencies are automatically detected, but it might need
# fine tuning.
buildOptions = dict(packages = [], excludes = [])

base = 'Console'

executables = [
    Executable('cmdact.py', base=base)
]

setup(name='cmdact',
      version = '1.0',
      description = 'sjadfioweafjawefasf',
      options = dict(build_exe = buildOptions),
      executables = executables)
