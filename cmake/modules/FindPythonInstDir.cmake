EXECUTE_PROCESS(COMMAND ${PYTHON_EXECUTABLE} -c "
from sys import stdout
from distutils import sysconfig
path=sysconfig.get_python_lib(True, prefix='${CMAKE_INSTALL_PREFIX}')
stdout.write(path)"
OUTPUT_VARIABLE PYTHON_INSTALL_DIR)
