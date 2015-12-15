# -*- coding: utf-8 -*-
import traceback

from ctypes import CFUNCTYPE, c_ulong, c_ulonglong, c_void_p, c_int, c_char_p, c_uint16, py_object, c_uint, c_uint32, c_uint64, c_float, POINTER, Structure

try:
    import ctypes
    import ctypes.util

    libpath = ctypes.util.find_library('xxx')
    lib = ctypes.CDLL(libpath)

    def lib_get_crypto_key_cb(userargp, status):
         userargp.cb_get_crypto_key(status)
         
    t_get_crypto_key_cb = CFUNCTYPE(None, py_object, c_uint)

    lib.get_crypto_key_cb_set.argtypes = [t_get_crypto_key_cb , py_object]
    lib.get_crypto_key_cb_set.restype = ctypes.c_int

    lib.run.argtypes = []
    lib.run.restype = None

except AttributeError, e:
    lib = None
    raise ImportError('ssh shared library not found or incompatible (%s)' % traceback.format_exc(e))
except Exception, e:
    lib = None
    import traceback
    raise ImportError('ssh shared library not found.\n'
                      'you probably had not installed libssh library.\n %s\n' % traceback.format_exc(e))

print "running"

class MyClass(object):
    def cb_get_crypto_key(self, status):
        print "Callback!!! %d" % status

myclass = MyClass()

lib.get_crypto_key_cb_set(t_get_crypto_key_cb(lib_get_crypto_key_cb), myclass)
lib.run()

print "running done"


