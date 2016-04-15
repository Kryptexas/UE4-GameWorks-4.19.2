#!/bin/bash

#set -x

ZLIB_VERSION=v1.2.8
SSL_VERSION=OpenSSL_1_0_1s
CURL_VERSION=curl-7_48_0
WS_VERSION=v1.7.4

# ================================================================================
configure_platform()
{
	# you can force SYSTEM and MACHINE here...
	# e.g. SYSTEM=$ANDROID_ARCH
	SYSTEM=$(uname)
	MACHINE=$(uname -m)

	# in other words, where to output the build
	case $SYSTEM in
		Linux)
			if [ $MACHINE == "x86_64" ]; then
				PLATFORM=Linux/x86_64-unknown-linux-gnu
			else
				PLATFORM=Linux/arm-unknown-linux-gnueabihf
			fi
			;;
		Darwin)
			PLATFORM=Mac
			;;
		*_NT-*)
			if [ $USE_VS_2013 ]; then
				PLATFORM=Win64/VS2013
			else
				PLATFORM=Win64/VS2015
			fi
			;;
		arch-arm)
			PLATFORM=Android/ARMv7
			;;
		arch-x86)
			PLATFORM=Android/x86
			;;
		arch-x86_64)
			PLATFORM=Android/x64
			;;
		*)
			PLATFORM=unknown
			;;
	esac
}

# ================================================================================

get_zlib()
{
	mkdir -p zlib
	cd zlib
		git clone https://github.com/madler/zlib.git $ZLIB_VERSION
		cd $ZLIB_VERSION
		git checkout $ZLIB_VERSION -b $ZLIB_VERSION
	cd ../..
}
path_zlib()
{
	cd zlib/$ZLIB_VERSION
	# ----------------------------------------
	CUR_PATH=$(pwd)
	CMAKE_PATH="$CUR_PATH/"
	BUILD_PATH="$CUR_PATH/Intermediate/$PLATFORM"
	ZLIB_ROOT_PATH="$CUR_PATH/INSTALL.$ZLIB_VERSION/$PLATFORM"
	ZLIB_ZLIB=libz.a
	if [[ $SYSTEM == *"_NT-"* ]]; then
		ZLIB_ZLIB=zlibstatic.lib
	fi
	# ----------------------------------------
	cd -
}
build_zlib()
{
	path_zlib

	cd zlib/$ZLIB_VERSION
	# ----------------------------------------
	# reset
	rm -f "$CMAKE_PATH"/CMakeCache.txt
	rm -rf "$ZLIB_ROOT_PATH"
	rm -rf "$BUILD_PATH"
	mkdir -p "$BUILD_PATH"
	cd "$BUILD_PATH"
	# ----------------------------------------
	echo 'Generating libwebsockets makefiles'
	"$CMAKE" -G "$CMAKE_GEN" \
		-DCMAKE_INSTALL_PREFIX:PATH="$ZLIB_ROOT_PATH" \
		"$CMAKE_PATH"

	# also "install" - will be used with libssl & libcurl
	case $SYSTEM in
		*_NT-*)
			"$CMAKE" --build . --config RelWithDebInfo
			"$CMAKE" --build . --config RelWithDebInfo --target install
			;;
		*)
			make
			make install
			;;
	esac
	# ----------------------------------------
	cd "$CUR_PATH/../.."
}
# OpenSSL on linux expects zlib compiled with -fPIC
path_zlib_fPIC()
{
	if [ $SYSTEM == 'Linux' ]; then
		cd zlib/$ZLIB_VERSION
		# ----------------------------------------
		CUR_PATH=$(pwd)
		CMAKE_PATH="$CUR_PATH/"
		BUILD_PATH="$CUR_PATH/Intermediate/fPIC/$PLATFORM"
		ZLIB_ROOT_PATH="$CUR_PATH/INSTALL.$ZLIB_VERSION/$PLATFORM"
		ZLIB_ZLIB=libz_fPIC.a
		# ----------------------------------------
		cd -
	fi
}
build_zlib_fPIC()
{
	if [ $SYSTEM == 'Linux' ]; then
		path_zlib_fPIC

		cd zlib/$ZLIB_VERSION
		# ----------------------------------------
		rm -f "$CMAKE_PATH"/CMakeCache.txt
#		rm -rf "$ZLIB_ROOT_PATH"
		rm -rf "$BUILD_PATH"
		mkdir -p "$BUILD_PATH"
		cd "$BUILD_PATH"
		# ----------------------------------------
		# change target from libz.* to libz_fPIC.* and enable PIC build
		git checkout -- "$CMAKE_PATH/CMakeLists.txt"
		perl -pi -e 's/(OUTPUT_NAME) z\)/$1 z_fPIC\)/' "$CMAKE_PATH/CMakeLists.txt"
		# ........................................
		"$CMAKE" -G "$CMAKE_GEN" \
			-DCMAKE_INSTALL_PREFIX:PATH="$ZLIB_ROOT_PATH" \
			-DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=TRUE \
			"$CMAKE_PATH"
		make
		make install
		# ----------------------------------------
		cd "$CUR_PATH/../.."
	fi
}
deadcode_zlib_graveyard()
{
	# OSX sed also requires backup file postfix/extension
	sed -i 's/\(OUTPUT_NAME\) z)/\1 z_fPIC\)\n\tset \(CMAKE_POSITION_INDEPENDENT_CODE TRUE\)/' "$CMAKE_PATH/CMakeLists.txt"

	make DESTDIR="$BUILD_PATH/INSTALL" install
#	$VISUALSTUDIO zlib.sln /build RelWithDebInfo
}

# ================================================================================

get_openssl()
{
	mkdir -p OpenSSL
	cd OpenSSL
		git clone https://github.com/openssl/openssl.git $SSL_VERSION
		cd $SSL_VERSION
		git checkout $SSL_VERSION -b $SSL_VERSION
	cd ../..
}
path_openssl()
{
	cd OpenSSL/$SSL_VERSION
	# ----------------------------------------
	CUR_PATH=$(pwd)
	CMAKE_PATH="$CUR_PATH/"
	BUILD_PATH="$CUR_PATH/Intermediate/$PLATFORM"
	SSL_ROOT_PATH="$CUR_PATH/INSTALL.$SSL_VERSION/$PLATFORM"
	# ----------------------------------------
	cd -
}
build_openssl()
{
	# WARNING: OpenSSL build will BOMB if $ZLIB_ROOT_PATH (as well as OpenSSL itself)
	#          contains any whitespace -- will revisit this at a later date...


	path_zlib
	path_zlib_fPIC
	path_openssl

	cd OpenSSL/$SSL_VERSION
	# ----------------------------------------
	# NOTE: ./config could have been used to try to "determine the OS" and run ./Configure
	# but, android-x86_64 needs to be added to ./Configure
	case $SYSTEM in
		Linux)
			SSL_ARCH=linux-x86_64
#			SSL_ARCH=linux-x86_64-clang
			;;
		Darwin)
			SSL_ARCH=darwin64-x86_64-cc
			;;
		*_NT-*)
			SSL_ARCH=VC-WIN64A
#			SSL_ARCH=debug-VC-WIN64A
			;;
		arch-arm)
			SSL_ARCH=android-armv7
			;;
		arch-x86)
			SSL_ARCH=android-x86
			;;
		arch-x86_64)
			SSL_ARCH=android-x86_64
			# ----------------------------------------
			# Add android-x86_64 config
			git checkout -- Configure
			perl -pi -e '/"android"/a \"android-x86_64\",\"gcc:-mandroid -I\\\$(ANDROID_DEV)\/include -B\\\$(ANDROID_DEV)\/lib -O2 -fomit-frame-pointer -fno-exceptions -fdata-sections -ffunction-sections -fno-short-enums -march=atom -fsigned-char -fPIC -Wall -MD -MF /dev/null -x c::-D_REENTRANT::-ldl:SIXTY_FOUR_BIT_LONG RC4_CHUNK DES_INT DES_UNROLL:\${no_asm}:dlfcn:linux-shared:-fPIC:-nostdlib -Wl,-shared,-Bsymbolic -Wl,--no-undefined -march=atom:.so.\\\$(SHLIB_MAJOR).\\\$(SHLIB_MINOR)\",' Configure
			;;
#		*IOS*)
#			SSL_ARCH=iphoneos-cross
#			;;
		*)
			;;
	esac


	# ----------------------------------------
	# reset
	rm -rf "$SSL_ROOT_PATH"
	rm -rf "$BUILD_PATH"
	mkdir -p "$BUILD_PATH"

	# ----------------------------------------
	echo "Configuring $SSL_VERSION for $SSL_ARCH"
	case $SYSTEM in
		# NOTE: [no-ssl2] option has been deprecated
		# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		*_NT-*)
			# https://github.com/openssl/openssl/issues/174#issuecomment-78445306
			# - the following would have also worked (which must be done after ./Configure ...):
			# - dos2unix "*/Makefile */*/Makefile */*.h */*/*.h"
			# to future-proof this, doing it with just git (for all files):
			git config --local core.autocrlf false
			git config --local core.eol lf
			git rm --cached -r .
			git reset --hard HEAD
			# this fix is needed when using git perl
			perl -pi -e 's!(dir})(\.\.)!\1/\2!' ms/uplink-x86_64.pl
			
			# WARNING: [threads] & [shared] options are not available on windows
			#          - errors on file generators
			#          - static & shared: are built separately via nt.mak & ntdll.mak
			# WARNING: no double quotes on zlib path - conflicts with C MACROS
			# WARNING: --with-zlib-lib is zlib itself -- whereas other (i.e. unix) builds is path
			# WARNING: no-asm means no need to install NASM -- OpenSSL does not support MASM
			#          https://github.com/openssl/openssl/blob/master/NOTES.WIN#L19-L22
			DOS_ZLIB_ROOT_PATH=$(cygpath -w $ZLIB_ROOT_PATH)
			DOS_SSL_ROOT_PATH=$(cygpath -w $SSL_ROOT_PATH)
			./Configure zlib no-asm \
				--with-zlib-lib=$DOS_ZLIB_ROOT_PATH\\lib\\$ZLIB_ZLIB \
				--with-zlib-include=$DOS_ZLIB_ROOT_PATH\\include \
				--prefix="$DOS_SSL_ROOT_PATH" \
				--openssldir="$DOS_SSL_ROOT_PATH" \
				$SSL_ARCH | tee "$BUILD_PATH/openssl-configure.log"
( cat <<_EOF_
			:: https://wiki.openssl.org/index.php/Compilation_and_Installation#W64

			call $VCVARSALL
			call ms\\do_win64a.bat

			nmake -f ms\\nt.mak    clean
			nmake -f ms\\ntdll.mak clean

			nmake -f ms\\nt.mak
			nmake -f ms\\ntdll.mak

			:: sanity check the ***SSL*** build
			copy "$DOS_ZLIB_ROOT_PATH\\bin\\zlib.dll" out32\\.
			copy "$DOS_ZLIB_ROOT_PATH\\bin\\zlib.dll" out32dll\\.
			nmake -f ms\\nt.mak    test
			nmake -f ms\\ntdll.mak test

			:: "install" - will use this with libcurl
			nmake -f ms\\nt.mak    install
			nmake -f ms\\ntdll.mak install

			:: double check these are indeed 64 bit
			dumpbin "$DOS_SSL_ROOT_PATH\\lib\\libeay32.lib" /headers | findstr machine
			dumpbin "$DOS_SSL_ROOT_PATH\\lib\\ssleay32.lib" /headers | findstr machine
_EOF_
) > dos_openssl.bat
			./dos_openssl.bat
			;;
		# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		*)
			./Configure shared threads zlib \
				--with-zlib-lib=$ZLIB_ROOT_PATH/lib \
				--with-zlib-include=$ZLIB_ROOT_PATH/include \
				--prefix="$SSL_ROOT_PATH" \
				--openssldir="$SSL_ROOT_PATH" \
				$SSL_ARCH | tee "$BUILD_PATH/openssl-configure.log"
			if [ $SYSTEM == "Linux" ]; then
				# OpenSSL on linux expects zlib built with -fPIC
				# which we made it as "z_fPIC"
				git checkout -- Makefile
				perl -pi -e 's/ -lz/ -lz_fPIC/' Makefile
			fi
			# separating builds as it was done before...
			make depend | tee "$BUILD_PATH/openssl-depend.log"
			make clean
			make build_crypto | tee "$BUILD_PATH/openssl-crypto.log"
			make build_ssl | tee "$BUILD_PATH/openssl-lib.log"

			# run SSL tests
			cp $ZLIB_ROOT_PATH/lib/*.so* .
			make test | tee "$BUILD_PATH/openssl-test.log"

			# "install" - will use this with libcurl
			make install | tee "$BUILD_PATH/openssl-install.log"
			;;
	esac
	# ----------------------------------------
	cd "$CUR_PATH/../.."
}

# ================================================================================

get_libcurl()
{
	mkdir -p libcurl
	cd libcurl
		git clone https://github.com/curl/curl.git $CURL_VERSION
		cd $CURL_VERSION
		git checkout $CURL_VERSION -b $CURL_VERSION
	cd ../..
}
path_libcurl()
{
	cd libcurl/$CURL_VERSION
	# ----------------------------------------
	CUR_PATH=$(pwd)
	CMAKE_PATH="$CUR_PATH/"
	BUILD_PATH="$CUR_PATH/Intermediate/$PLATFORM"
	CURL_ROOT_PATH="$CUR_PATH/INSTALL.$CURL_VERSION/$PLATFORM"
	# ----------------------------------------
	cd -
}
build_libcurl()
{
	path_zlib
	path_zlib_fPIC
	path_openssl
	path_libcurl

	cd libcurl/$CURL_VERSION
	# ----------------------------------------
	# reset
	rm -f "$CMAKE_PATH"/CMakeCache.txt
	rm -rf "$CURL_ROOT_PATH"
	rm -rf "$BUILD_PATH"
	mkdir -p "$BUILD_PATH"
#	cd "$BUILD_PATH"
	# ----------------------------------------
	echo 'Generating curl makefiles'
	case $SYSTEM in
		# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		*_NT-*)
			rm -rf builds
			# build details from ./winbuild/BUILD.WINDOWS.txt
			./buildconf.bat
			DEPS=$BUILD_PATH/WITH_DEVEL_DEPS
			mkdir -p $DEPS/{bin,include,lib}
			cp -r {$SSL_ROOT_PATH,$ZLIB_ROOT_PATH}/bin/* $DEPS/bin/.
			cp -r {$SSL_ROOT_PATH,$ZLIB_ROOT_PATH}/include/* $DEPS/include/.
			cp -r {$SSL_ROOT_PATH,$ZLIB_ROOT_PATH}/lib/* $DEPS/lib/.
			DOS_DEPS=$(cygpath -w $DEPS)
			cd winbuild
			# our static zlib is named zlibstatic
			git checkout -- MakefileBuild.vc
			perl -pi -e 's/zlib_a/zlibstatic/' MakefileBuild.vc

			if [ $USE_VS_2013 ]; then
				VC=12
			else
				VC=14
			fi
( cat <<_EOF_
			call $VCVARSALL
			nmake -f Makefile.vc mode=static VC=$VC WITH_DEVEL=$DOS_DEPS WITH_SSL=static WITH_ZLIB=static GEN_PDB=yes MACHINE=x64
			nmake -f Makefile.vc mode=dll    VC=$VC WITH_DEVEL=$DOS_DEPS WITH_SSL=static WITH_ZLIB=static GEN_PDB=yes MACHINE=x64
_EOF_
) > dos_libcurl.bat
			./dos_libcurl.bat
			cd -
			# make install
			mkdir -p $CURL_ROOT_PATH
			mv builds/* $CURL_ROOT_PATH/.
			;;
		# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		*)
			if [ $SYSTEM == 'Linux' ]; then
				# OpenSSL on linux expects zlib built with -fPIC
				# which we made it as "z_fPIC"
				git checkout -- configure.ac
				perl -pi -e 's/-lz/-lz_fPIC/' configure.ac
			fi
			# build details from ./GIT-INFO
			./buildconf
			./configure \
				--with-zlib=$ZLIB_ROOT_PATH \
				--with-ssl=$SSL_ROOT_PATH \
				--prefix="$CURL_ROOT_PATH" \
				--enable-static --enable-shared \
				--enable-threaded-resolver --enable-hidden-symbols \
				--disable-ftp --disable-file --disable-ldap --disable-ldaps \
				--disable-rtsp --disable-telnet --disable-tftp \
				--disable-dict --disable-pop3 --disable-imap --disable-smtp \
				--disable-gopher --disable-manual --disable-idn | tee "$BUILD_PATH/curl-configure.log"
			env LDFLAGS=-L$SSL_ROOT_PATH/lib make | tee "$BUILD_PATH/curl-make.log"
			make install
			;;
	esac
	# ----------------------------------------
	cd "$CUR_PATH/../.."
}
deadcode_libcurl_graveyard()
{
	# CURL says "cmake build system is poorly maintained"...
	cd "$BUILD_PATH"
	"$CMAKE" -G "$CMAKE_GEN" \
		-DCMAKE_INSTALL_PREFIX:PATH="$CURL_ROOT_PATH" \
		-DOPENSSL_ROOT_DIR:PATH="$SSL_ROOT_PATH" \
		-DOPENSSL_INCLUDE_DIR:PATH="$SSL_ROOT_PATH/include" \
		-DOPENSSL_LIBRARIES:PATH="$SSL_ROOT_PATH/lib" \
		"$CMAKE_PATH"

# OSX bash is old -- doesn't support case-statement fall-through
#		Linux)
#			git checkout -- configure.ac
#			perl -pi -e 's/-lz/-lz_fPIC/' configure.ac
#			;&
#		*)
#			...
#			;;
}

# ================================================================================

get_libwebsockets()
{
	mkdir -p libWebSockets
	cd libWebSockets
		git clone https://github.com/warmcat/libwebsockets.git $WS_VERSION
		cd $WS_VERSION
		git checkout $WS_VERSION -b $WS_VERSION
	cd ../..
}
path_libwebsockets()
{
	cd libWebSockets/$WS_VERSION
	# ----------------------------------------
	CUR_PATH=$(pwd)
	CMAKE_PATH="$CUR_PATH/"
	BUILD_PATH="$CUR_PATH/Intermediate/$PLATFORM"
	WS_ROOT_PATH="$CUR_PATH/INSTALL.$WS_VERSION/$PLATFORM"
	# ----------------------------------------
	cd -
}
build_libwebsockets()
{
	path_zlib
	path_zlib_fPIC
	path_openssl
	path_libwebsockets

	cd libWebSockets/$WS_VERSION
	# ----------------------------------------
	# reset
	rm -f "$CMAKE_PATH"/CMakeCache.txt
	rm -rf "$WS_ROOT_PATH"
	rm -rf "$BUILD_PATH"
	mkdir -p "$BUILD_PATH"
	cd "$BUILD_PATH"

	# ----------------------------------------
	echo 'Generating libwebsockets makefiles'
	case $SYSTEM in
		*_NT-*)
			if [ $USE_VS_2013 ]; then
				# TODO: depricate this when visual studio 2013 is no longer supported
				# https://connect.microsoft.com/VisualStudio/feedback/details/809403/error-c3861-snprintf-identifier-not-found-in-visual-studio-2013
				# - when using VS 2013: test code does not have snprintf (nor LWS_HAVE__SNPRINTF) defined
				FILES=(test-fraggle.c test-ping.c test-server.h)
				for f in ${FILES[@]}; do
					git checkout -- $CUR_PATH/test-server/${f}
					perl -pi -e 's/(libwebsockets.h")/\1\n#define snprintf _snprintf/' $CUR_PATH/test-server/${f}
				done
			fi
			# NOTE: windows only: tell cmake to not use bundled zlib
			"$CMAKE" -G "$CMAKE_GEN" \
				-DCMAKE_INSTALL_PREFIX:PATH="$WS_ROOT_PATH" \
				-DOPENSSL_ROOT_DIR:PATH="$SSL_ROOT_PATH" \
				-DLWS_USE_BUNDLED_ZLIB:BOOL=OFF \
				-DLWS_ZLIB_INCLUDE_DIRS:PATH="$ZLIB_ROOT_PATH"/include \
				-DLWS_ZLIB_LIBRARIES:PATH="$ZLIB_ROOT_PATH"/lib/$ZLIB_ZLIB \
				"$CMAKE_PATH"
			"$CMAKE" --build . --config RelWithDebInfo
			"$CMAKE" --build . --config RelWithDebInfo --target install
			;;
		*)
			# NOTE: https://github.com/warmcat/libwebsockets/blob/master/README.build.md
			#       unix only: need: -DCMAKE_INCLUDE_DIRECTORIES_PROJECT_BEFORE
			"$CMAKE" -G "$CMAKE_GEN" \
				-DCMAKE_INSTALL_PREFIX:PATH="$WS_ROOT_PATH" \
				-DCMAKE_INCLUDE_DIRECTORIES_PROJECT_BEFORE:PATH="$SSL_ROOT_PATH" \
				-DOPENSSL_ROOT_DIR:PATH="$SSL_ROOT_PATH" \
				-DLWS_ZLIB_INCLUDE_DIRS:PATH="$ZLIB_ROOT_PATH"/include \
				-DLWS_ZLIB_LIBRARIES:PATH="$ZLIB_ROOT_PATH"/lib/$ZLIB_ZLIB \
				"$CMAKE_PATH"
			make
			make install
			;;
	esac
	# ----------------------------------------
	cd "$CUR_PATH/../.."
}

# ================================================================================

reset_hard()
{
	# ........................................
	cd zlib/$ZLIB_VERSION
		git reset --hard HEAD
		git clean -fd
	cd ../..
	# ........................................
	cd OpenSSL/$SSL_VERSION
		git reset --hard HEAD
		git clean -fd
	cd ../..
	# ........................................
	cd libcurl/$CURL_VERSION
		git reset --hard HEAD
		git clean -fd
	cd ../..
	# ........................................
	cd libWebSockets/$WS_VERSION
		git reset --hard HEAD
		git clean -fd
	cd ../..
	# ........................................
}

build_environment()
{
	# default to unix centric
	CMAKE_GEN='Unix Makefiles'
	CMAKE=cmake

	case $SYSTEM in
		Darwin)
			# remember to:
			# brew install autoconf automake libtool cmake git
			;;
		*_NT-*)
			# this script was tested on GIT BASH
			# MS VisualStudio and CMAKE are also required
			if [ $USE_VS_2013 ]; then
				CMAKE_GEN='Visual Studio 12 2013 Win64'
				VCVARSALL="\"$VS120COMNTOOLS..\\..\\VC\\vcvarsall.bat\" x86_amd64"
#				VISUALSTUDIO="$VS120COMNTOOLS..\\IDE\\WDExpress.exe"
			else
				CMAKE_GEN='Visual Studio 14 2015 Win64'
				VCVARSALL="\"$VS140COMNTOOLS..\\..\\VC\\vcvarsall.bat\" amd64"
#				VISUALSTUDIO="$VS140COMNTOOLS..\\IDE\\devenv.exe"
			fi
			CMAKE="C:\\Program Files (x86)\\CMake\\bin\\cmake.exe"
			;;
		*)
			;;
	esac
}

# ================================================================================
# MAIN
# ================================================================================

get_zlib
get_openssl
get_libcurl
get_libwebsockets
	#reset_hard

configure_platform
build_environment

build_zlib
build_zlib_fPIC
build_openssl
build_libcurl
build_libwebsockets

if [[ $SYSTEM == *'_NT-'* ]]; then
	# as of Unreal Engine 4.11 -- Visual Studio 2013 is still supported
	echo building with Visual Studio 2013
	USE_VS_2013=1
	configure_platform
	build_environment

	build_zlib
	build_openssl
	build_libcurl
	build_libwebsockets
fi

# handy commands to know:
# cmake -LAH .
# ./configure --help
# git show-ref --head $TAG

