from distutils.core import setup, Extension

eugene_sysv = Extension('eugene_sysv',
        sources = ['eugene_sysv.c'])

setup ( name = 'EugeneSysV',
	version = '1.0',
	description = 'Eugene support module.',
	ext_modules = [ eugene_sysv ])
