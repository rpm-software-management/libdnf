all: example_plugin

example_plugin: example_plugin.c
	gcc -fPIC -shared -I/usr/include/glib-2.0 -I/lib64/glib-2.0/include -Wcast-align -Wno-uninitialized -Wredundant-decls -Wwrite-strings -Wformat-nonliteral -Wmissing-format-attribute -Wsign-compare -Wtype-limits -Wuninitialized -Wall -Werror=implicit-function-declaration -Wl,--as-needed -Wmissing-prototypes -Waggregate-return -Wshadow -o example_plugin.so example_plugin.c
