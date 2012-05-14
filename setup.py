from distutils.core import setup, Extension

liblvm = Extension('lvm',
                    sources = ['liblvm.c'],
                    libraries= ['lvm2app'])

setup (name = 'lvm',
       version = '1.2',
       description = 'Python binding for liblvm2',
       maintainer='Andy Grover',
       maintainer_email='andy@groveronline.com',
       url='http://github.com/agrover/python-lvm',
       ext_modules = [liblvm])
