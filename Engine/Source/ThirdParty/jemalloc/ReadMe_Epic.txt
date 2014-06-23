Headers are generated during configure stage and may differ per platform. Current headers for Linux have been generated
for a x86-64 CentOS 6.4 (but they are compatible with RHEL 6.4 and x86-64 Linuxes in general).

For Linux platform, the library is supplied in two forms:

- "separate" means that function names are mangled and libc malloc()/free() is still available 
   for use (by other libraries or by the rest of code, e.g. FMallocAnsi).

- "drop-in" means that jemalloc replaces libc malloc()/free() and every allocation is made through it - including 
   libraries like libcurl and *also* FMallocAnsi.  This may have lower memory footprint overall.

If you want to build a newer version of library, see build/HowToBuild_Epic.txt

-RCL
