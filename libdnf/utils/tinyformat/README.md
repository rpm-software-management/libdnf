# tinyformat.h

## A minimal type safe printf() replacement

**tinyformat.h** is a type safe printf replacement library in a single C++
header file.  If you've ever wanted `printf("%s", s)` to just work regardless
of the type of `s`, tinyformat might be for you.  Design goals include:

* Type safety and extensibility for user defined types.
* C99 `printf()` compatibility, to the extent possible using `std::ostream`
* Simplicity and minimalism.  A single header file to include and distribute
  with your projects.
* Augment rather than replace the standard stream formatting mechanism
* C++98 support, with optional C++11 niceties

Original project URL:
https://github.com/c42f/tinyformat

Added:
* POSIX extension for positional arguments
