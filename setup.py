from distutils.core import setup, Extension

liblvm = Extension('liblvm',
                    sources = ['liblvm.c'],
                    libraries= ['lvm2app'])

setup (name = 'liblvm',
       version = '1.0',
       description = 'Python binding for liblvm2',
       ext_modules = [liblvm])
