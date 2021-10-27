This process is not for the faint of heart.  There are a lot of tedious steps involved.  There are two major "phases."  The first phase builds a cross-binutils and a cross-compiler that runs on the build system (assuming a modern Linux system) to generate binaries for the target SPARC v8 Solaris platform.  The second phase uses the components built in the first phase to build native binutils and a native compiler that will run on the SPARC v8 Solaris platform.

Some nomenclature used throughout this document:

- target system: For the compiler and binutils, this is the platform (CPU and operating system) for which it will generate binaries.
- host system: The system on which the generated program will run.  A cross-compiler will have a different host and target.  It runs on the host, but it generates binaries for the target.
- build system: The system on which programs are being built.  When using a cross-compiler (host and target are different) to generate a native compiler (host and target are the same), the build system will be the same as the host for the cross-compiler.  It will be different than the target for the cross-compiler or the native compiler, and it will be different than host for the native compiler.

The reference target (and ultimately host) system used for this document is a SPARCstation 5 running Solaris 2.6.  Names and locations of files may differ on other versions of Solaris.  Platform-specific flags may differ for other SPARC processors.  The reference build system used for this document is a x86-64 running Fedora 33.

Much of the information in this document is based on [How to Build a GCC Cross-Compiler](https://preshing.com/20141119/how-to-build-a-gcc-cross-compiler/).

# Phase 0: Setup

Before starting down this long and winding road, there is a bunch of software that must be collected both from the Internet and from the Solaris system.

Testing cross-binuntils and building the cross-compiler require some files from the Solaris system.  Specifically, the contents of `/usr/lib` and `/usr/include` are needed.  Collect those files using the following commands:

        cd /usr
        tar cf $HOME/usr-lib-include.tar include lib

Some files from `/usr/ccs/lib` are also needed for some steps of the build.  Specifically, `values-Xa.o` is needed to build the native compiler, and `libtermcap.a` is needed to build the native binutils (for GDB).

        cd /usr/ccs/lib
        tar cf $HOME/ccs-lib.tar lib*.a values-*.o

Transfer the resulting `usr-lib-include.tar` and `ccs-lib.tar` files to the build system.

The complete compiler suite includes GNU binutils and the GNU compiler collection.  Binutils from GIT was found to work with only trivial issues (discussed in the 'Building the cross-tools' section).  The `releases/gcc-4.3` branch from this repository should be used for GCC.  Several code changes were necessary for GCC 4.3, and those are included in this repository.

In addition, building GCC requires `mpfr` and `gmp`.  On Fedora 33, the development packages installed on the system were sufficient to build the cross-compiler.  Building these packages from source code was only necessary to build the native compiler.

All of the relevant code can be acquired with the following commands:

        wget http://ftpmirror.gnu.org/mpfr/mpfr-3.1.2.tar.xz 
        wget http://ftpmirror.gnu.org/gmp/gmp-6.0.0a.tar.xz 
        git clone git://sourceware.org/git/binutils-gdb.git 
        git clone https://github.com/ianromanick/gcc.git 

It is possible to newer versions of `mpfr`, `gmp`, and `mpc` will also work.  The versions listed here were used in the article referenced above for building GCC 4.9.2.

# Phase 1: Building the cross-tools

All of the cross-tools will be installed in `~/sparc`.  The first step is to partially populate this directory with the files acquired from the Solaris machine.

        cd ~
        mkdir -p sparc/sparc-sun-solaris2
        cd sparc/sparc-sun-solaris2
        tar xf ~/usr-lib-include.tar
        mv include sys-include
        cd lib
        tar xf ~/ccs-lib.tar

## Building cross-binutils

The first major piece of software to build is binutils.  At least as of `b3a01ce2155` ("CTF: incorrect underlying type setting for enumeration types"), binutils GIT seems to work properly without major problems.

        mkdir binutils-gdb-sparc-sun-solaris2.6
        cd binutils-gdb-sparc-sun-solaris2.6
        ../binutils-gdb/configure --prefix=$HOME/sparc --target=sparc-sun-solaris2.6 \
            --host=$MACHTYPE --build=$MACHTYPE --disable-gold --disable-gdb \
             --disable-multilib
        make && make install

The gold linker is disabled because it won't be necessary on the target, and GDB is disabled because it's not useful to run GDB for the target system on the build system.  Multilib is disabled because 64-bit libraries are not possible on the target platform.

At this point it is possible to assemble and link a small program to run on the target system!  Copy and paste the following assembly program to a file called `a.s`.

                .global main
        main:
                save    %sp, -112, %sp
                mov     15, %o2
                mov     1, %o0
                sethi   %hi(msg), %o1
                mov     0, %i0
                call    write, 0
                 or     %o1, %lo(msg), %o1
                jmp     %i7+8
                 restore
        msg:
                .asciz  "Hello, world!\n"

Assemble and link the program with the following commands:

        sparc-sun-solaris2.6-as a.s -o a.o
        sparc-sun-solaris2.6-ld a.o -lc -o hello1

This should result in a `hello1` file that is in the neighborhood of 2,100 bytes.  More importantly, the `file` command should describe it as "`ELF 32-bit MSB executable, SPARC, version 1 (SYSV), dynamically linked, interpreter /usr/lib/ld.so.1, not stripped`".  Copy the `hello1` file to the Solaris sysem, and run it there.  It should produce the output, "Hello, world!"

Assuming the simple program works as expected on the Solaris system, it is time to move on to building the cross-compiler.

## Building cross-GCC

Building the cross-compiler proceeds in a manner similar to building the cross-binutils.  There is one additional hitches.  At some point between 2011 and 2021, the texinfo tools changed in a backwards incompatible way.  As a result, 2021 texinfo tools cannot build the documenation for GCC 4.3.  This can probably be fixed, but the root cause of the problem as not been investigated.  A workaround is to replace the `makeinfo` command with the `true` command.  The `make` commandline shown below works around this issue:

        cd gcc
        git checkout releases/gcc-4.3
        cd ..
        mkdir gcc-sparc-sun-solaris2.6
        cd gcc-sparc-sun-solaris2.6
        ../gcc/configure --prefix=~/sparc --target=sparc-sun-solaris2.6 \
            --host=$MACHTYPE --build=$MACHTYPE --enable-obsolete \
            --enable-languages='c,c++'
        make MAKEINFO=/usr/bin/true && \
            make MAKEINFO=/usr/bin/true install

Only the compilers for C and C++ are built because those are the only languages needed to build the native compiler.

At this point it is possible to compile a small program to run on the target system!  Copy and paste the following C program to a file called `b.c`.

        #include <stdio.h>
        
        int main(int argc, char **argv)
        {
            printf("Hello, world!\n");
            return 0;
        }

Compile the program with the following command:

        sparc-sun-solaris2.6-gcc b.c -o hello2

This should result in a `hello2` file that is in the neighborhood of 6,300 bytes.  More importantly, the `file` command should describe it as

        ELF 32-bit MSB executable, SPARC, version 1 (SYSV), dynamically linked, interpreter /usr/lib/ld.so.1, not stripped

Copy the `hello2` file to the Solaris system, and run it there.  It should produce the output, "Hello, world!"

Assuming the simple program works as expected on the Solaris system, it is time to move on to building the native compiler.  This is where the "fun" really begins.

# Phase 2: Building the native tools

All of the native tools will be installed in `/opt/local`.  Normally these tools would be installed on the target machine in `/usr/local`.  Since `/usr/local` on the build machine contains native binaries, it is not practical to build the programs to install there.  `/opt/local` is chosen because it is unlikely to confict with anything on the build system, and it should also work on the target system.  Using `chroot` is also possible, but it is requires more effort.

Prepare the installation directory using the following commands:

        sudo mkdir /opt/local
        sudo chown $(id -un):$(id -gn) /opt/local

## Building native binutils

Unfortunately, it is not possible to build current binutils with GCC 4.3 because C11 support is required.  Several versions of binutils that could build with GCC 4.3 have various problems building for Solaris 2.6.  The most recent version that works without too much effort is binutils 2.25.

Some subpackages of binutils will check for `stdint.h`, but others will blindly assume that it exists.  Copy-and-paste the following to `~/sparc/sparc-sun-solaris2/sys-include/stdint.h`.  In addition to providing the parts of `stdint.h` that the various programs and libraries need, it also works around a very strange quirk of `INTPTR_MAX` and `UINTPTR_MAX` in Solaris 2.6.

        /* Copyright (c) 2021 Ian Romanick. All rights reserved.
         *
         * This work is licensed under the terms of the MIT license.
         * For a copy, see <https://opensource.org/licenses/MIT>.
         */
        #ifndef STDINT_H
        #define STDINT_H
        #include <inttypes.h>

        /* Solaris 2.6 inttypes.h defines INTPTR_MAX and UINTPTR_MAX as empty
         * strings.  That causes problems with source code that expects them to be
         * numbers!
         */
        #if defined(UINTPTR_MAX)
        #  if UINTPTR_MAX - 0 == 0
        #     undef UINTPTR_MAX
        #  endif
        #endif
        #if defined(INTPTR_MAX)
        #  if INTPTR_MAX - 0 == 0
        #     undef INTPTR_MAX
        #  endif
        #endif
        
        #if !defined(UINTPTR_MAX)
        #  if defined(_LP64)
        #    define UINTPTR_MAX UINT64_MAX
        #  else
        #    define UINTPTR_MAX UINT32_MAX
        #  endif
        #endif
        #if !defined(INTPTR_MAX)
        #  if defined(_LP64)
        #    define INTPTR_MAX INT64_MAX
        #    define INTPTR_MIN INT64_MIN
        #  else
        #    define INTPTR_MAX INT32_MAX
        #    define INTPTR_MIN INT32_MIN
        #  endif
        #endif
        
        #if !defined(_BYTE_ORDER) && !defined(_LITTLE_ENDIAN) && defined(_BIG_ENDIAN)
        #define __BIG_ENDIAN__
        #elif !defined(_BYTE_ORDER) && defined(_LITTLE_ENDIAN) && !defined(_BIG_ENDIAN)
        #define __LITTLE_ENDIAN__
        #else
        #error Confusing byte-order situation.
        #endif
        
        #endif /* ifndef STDINT_H */

Many of the problems occur while building GDB.  Versions more recent than 2.25 may build if GDB is disabled via `--disable-gdb` configuration parameter.  However, GDB is the one tool from more recent binutils that is likely to have any additional utility, so this is unlikely to be useful.

        cd binutils-gdb
        git checkout binutils-2_25-branch
        cd ..
        mkdir binutils-gdb-sparc-sun-solaris2-native
        cd binutils-gdb-sparc-sun-solaris2-native
        CFLAGS="-I$HOME/sparc-sun-solaris2.6/sys-include -fno-strict-aliasing" \
            ../binutils-gdb/configure --prefix=/opt/local \
            --target=sparc-sun-solaris2.6 --host=sparc-sun-solaris2.6 \
            --build=$MACHTYPE --disable-largefile --disable-multilib
        make && make install
        cd ..

## Building `gmp` and `mpfr`

Building `gmp` and `mpfr` is straightforward.  Simply build `gmp` first using the following commands:

        tar xf gmp-6.0.0a.tar.xz
        mkdir gmp-sparc-sun-solaris2.6
        cd gmp-sparc-sun-solaris2.6
        ../gmp-6.0.0/configure --prefix=/opt/local --target=sparc-sun-solaris2.6 \
            --host=sparc-sun-solaris2.6 --build=$MACHTYPE
        make && make install
        cd ..

Then build `mpfr` with the following commands:

        tar xf mpfr-3.1.2.tar.xz
        mkdir mpfr-sparc-sun-solaris2.6
        cd mpfr-sparc-sun-solaris2.6
        ../mpfr-3.1.2/configure --prefix=/opt/local --target=sparc-sun-solaris2.6 \
            --host=sparc-sun-solaris2.6 --build=$MACHTYPE --with-gmp=/opt/local
        make && make install
        cd ..
## Building native GCC

The final turn before the end of the race approaches.

GCC is built in a very similar manner as before.  The major additions are the `--with-gmp` and `--with-mpfr` options that instruct configure to use those libraries and include files from `/opt/local`.

        mkdir gcc-sparc-sun-solaris2.6-native
        cd gcc-sparc-sun-solaris2.6-native
        ../gcc/configure --prefix=/opt/local --target=sparc-sun-solaris2.6 \
            --host=sparc-sun-solaris2.6 --build=$MACHTYPE --enable-obsolete \
            --enable-languages='c,c++' --with-gmp=/opt/local \
            --with-mpfr=/opt/local \
            --with-gnu-ld --with-gnu-as
        make MAKEINFO=/usr/bin/true && make MAKEINFO=/usr/bin/true install

Transfering the files to the Solaris system presents still more problems.  Several of the files have full paths that are longer than 99 characters, and Solaris 2.6 `tar` cannot handle that.  It will terminate with the error

        tar: directory checksum error

The majority of online sources (e.g., stackechange) will emphatically state that this is due to giving `tar` a compressed file, but that is not the only cause of this problem.  An [IBM support article][1] does, however, provide a correct solution without providing any analysis of the root cause.  There are two reasonable solutions.  As suggested in the IBM support article, cross-building GNU Tar for the target system will work.  Having GNU Tar on the target system is probably the best long-term solution.  Cross-building it should be accomplished in a similar manner as cross-building any of the other components built here.

An alternative workaround is the create two separate archive files.  The first contains all of the files except the files in `local/include/c++/4.3.6/ext/pb_ds/detail`, and the second contains only those files.  The second archive is created relative to that directory, so the full paths of those files are much, much shorter.

        cd /opt
        tar -c -H v7 --exclude='local/include/c++/4.3.6/ext/pb_ds/detail/*' \
            -f ~/everything.tar local 
        cd local/include/c++/4.3.6/ext/pb_ds
        tar -c -H v7 -f ~/just-detail.tar detail

Once the files are extracted on the target system there are a couple additional steps.

1. Change the ownership of all the files to `bin:bin` using `cd /opt/local ; chown -R bin:bin *`
2. Add `/opt/local/bin` to the user's `PATH` variable.  It is important that `/opt/local/bin` appears **before** `/usr/bin` in the path or GCC might try to use the system `as` instead of GNU `as`.  This will likely fail for some programs.
3. Add `/opt/local/lib` to the user's `LD_LIBRARY_PATH` variable.

Now it is possible to rebuild all of the tools on the target machine.  This is not recommended unless one just wants to marvel at how much faster computers have become in the last 20 years.

[1]: https://www.ibm.com/mysupport/s/question/0D50z00006PEBmN/why-do-i-get-a-checksum-error-when-untarring-a-tar-file-on-aix-?language=en_US
