from distutils.core import setup, Extension

eugene_win32 = Extension('eugene_win32',
		sources = ['eugene_win32.c'])

setup ( name = 'EugeneWin32',
	version = '1.0',
	description = 'Eugene support module.',
	ext_modules = [ eugene_win32 ])
