# libdnf template plugin

Template libdnf plugin with set up cmake and spec file.

## Getting started

In order to help kickstart your libdnf plugin you can:
* Run ```sed -i 's/template_plugin/your_name/g' template_plugin.c template_plugin.spec CMakeLists.txt```
* Rename template_plugin.c to your_name.c and template_plugin.spec to your_name.spec
* And modify your_name.c plugins source code to your liking

Then you can build as usual: ```mkdir build && cd build && cmake .. && make```

## Building simple RPM with default configs:

 * Pack source files: ``` tar -czvf ~/rpmbuild/SOURCES/your_name.tar.gz ../template/ ```
 * And run: ```rpmbuild --ba your_name.spec``` 
