from __future__ import absolute_import

# error needs to be imported first and with global visibility for its symbols,
# as it defines a python exception, which is a global variable and the other
# modules use the symbol.
import sys, os
sys.setdlopenflags(os.RTLD_NOW | os.RTLD_GLOBAL)
from . import error

# Other modules also need to be loaded with RTLD_GLOBAL to preserve uniqueness
# of RTTI.  There are code paths where an exception thrown in one module is
# supposed to be caught in another.
from . import common_types
from . import conf
from . import module
from . import repo
from . import transaction
from . import utils
sys.setdlopenflags(os.RTLD_NOW)
