from distutils.core import setup, Extension
import os

ESDK	=	os.environ['EPIPHANY_HOME']
ELIBS	=	ESDK + "/tools/host/lib"
EINCS	=	ESDK + "/tools/host/include"
ELDF	=	ESDK + "/bsps/current/fast.ldf"


e_mod = Extension(	'e_mod',
					sources 		= 	['e_src/e_mod.c','e_src/tools.c'],
					libraries 		= 	['e-hal','m'],
					library_dirs 	= 	[ELIBS],
					include_dirs 	= 	[EINCS],
					extra_compile_args	=	["-std=c99"])

setup (name = 'e_mod',
       version = '1.0',
       description = 'Interpolator Module',
       ext_modules = [e_mod])