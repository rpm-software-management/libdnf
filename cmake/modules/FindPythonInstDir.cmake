EXECUTE_PROCESS(COMMAND ${PYTHON_EXECUTABLE} -c "
from sys import stdout
from sysconfig import get_path
path=get_path(name='platlib', vars={'platbase':'${CMAKE_INSTALL_PREFIX}'})
stdout.write(path)"
OUTPUT_VARIABLE PYTHON_INSTALL_DIR)
