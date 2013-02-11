AC_INIT()
dnl #----------------------------------------------------------------
dnl # ArcSim configuration START
dnl # @http://www.gnu.org/software/autoconf/manual/autoconf.html
dnl #----------------------------------------------------------------

AC_CONFIG_AUX_DIR(config)
AC_CONFIG_HEADERS([inc/config.h])

AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE([arcsim], [1.0], [npt@inf.ed.ac.uk])

dnl ----------------------------------------------------------------
dnl Some simple checks.
dnl ----------------------------------------------------------------

AC_PROG_CC
AC_PROG_CXX
AC_PROG_RANLIB
AC_PROG_INSTALL

AC_CHECK_TYPES([sighandler_t, sig_t, __sighandler_t],,,
   [#include <sys/types.h>
    #include <signal.h>])

AC_CHECK_TYPE(loff_t,off_t)


dnl #----------------------------------------------------------------
dnl # Test if Compiler options are available
dnl #----------------------------------------------------------------
AC_DEFUN([AX_GCC_OPTION], [
  AC_REQUIRE([AC_PROG_CC])
  AC_MSG_CHECKING([if gcc accepts $1 option])
  AS_IF([ test "x$GCC" = "xyes" ],[
    AS_IF([ test -z "$3" ],[
      ax_gcc_option_test="int main() { return 0; }"
    ],[
      ax_gcc_option_test="$3"
    ])
    # Dump the test program to file
    cat <<EOF > conftest.c
$ax_gcc_option_test
EOF
    # Dump back the file to the log, useful for debugging purposes
    AC_TRY_COMMAND(cat conftest.c 1>&AS_MESSAGE_LOG_FD)
    AS_IF([ AC_TRY_COMMAND($CC $2 $1 -c conftest.c 1>&AS_MESSAGE_LOG_FD) ],[
            AC_MSG_RESULT([yes])
        $4
    ],[
            AC_MSG_RESULT([no])
        $5
    ])
  ],[
    AC_MSG_RESULT([no gcc available])
  ])
])

AX_GCC_OPTION([-flax-vector-conversions],[],[],
  [CFLAGS="$CFLAGS -flax-vector-conversions";
   CXXFLAGS="$CXXFLAGS -flax-vector-conversions"],
  [])

AX_GCC_OPTION([-Wshorten-64-to-32],[],[],
  [CFLAGS="$CXXWARNINGS -Wshorten-64-to-32";
   CXXFLAGS="$CXXWARNINGS -Wshorten-64-to-32"],
  [])

AX_GCC_OPTION([-Wnewline-eof],[],[],
  [CXXFLAGS="$CXXWARNINGS -Wnewline-eof"],
  [])


dnl #----------------------------------------------------------------
dnl # Choosing Arcsim Package Options: 
dnl # AC_ARG_ENABLE (feature, help-string, [action-if-given], [action-if-not-given])
dnl #----------------------------------------------------------------

AC_ARG_ENABLE(64bit,
  [AS_HELP_STRING([--enable-64bit],
                [Build 64bit version of simulator])],
  # action if given
  [MACHINE="-m64";
   ARCH="x86_64";
   AC_MSG_NOTICE([64-bit Simulator build enabled.])],
  # action if not given
  [MACHINE="-m32";
   ARCH="i386";
   AC_MSG_NOTICE([32-bit Simulator build enabled.])]
)

AC_ARG_ENABLE(llvm-30,
  [AS_HELP_STRING([--enable-llvm-30],
                [Build LLVM 3.0 compatible Simulator.])],
  # action if given
  [LLVM_VERSION="30";
   SIM_MODE="-DLLVM_VERSION=30 $SIM_MODE";
   AC_MSG_NOTICE([LLVM 3.0 Simulator build enabled.])],
  # action if not given
  [LLVM_VERSION="29";
   SIM_MODE="-DLLVM_VERSION=29 $SIM_MODE";
   AC_MSG_NOTICE([LLVM 2.9 Simulator build enabled.])]
)


AC_ARG_ENABLE(cycle-accurate,
  [AS_HELP_STRING([--enable-cycle-accurate],
                [Build cycle accurate enabled simulator])],
  [SIM_MODE="-DCYCLE_ACC_SIM $SIM_MODE";
   AC_MSG_NOTICE([Cycle accurate simulation mode enabled.])]
)

AC_ARG_ENABLE(bpu-model,
  [AS_HELP_STRING([--enable-bpu-model],
                [Build simulator supporting various BPU models.])],
  [SIM_MODE="-DENABLE_BPRED $SIM_MODE";
   AC_MSG_NOTICE([Branch predictor models enabled.])]
)

AC_ARG_ENABLE(cosim,
  [AS_HELP_STRING([--enable-cosim],
                [Build co-simulation enabled simulator])],
  [SIM_MODE="-DCOSIM_SIM $SIM_MODE";
   AC_MSG_NOTICE([Co-simulation mode enabled.])]
)

AC_ARG_ENABLE(regtrack,
  [AS_HELP_STRING([--enable-regtrack],
                [Build register tracking enabled simulator])],
  [SIM_MODE="-DREGTRACK_SIM $SIM_MODE";
   AC_MSG_NOTICE([Register tracking mode enabled.])]
)

AC_ARG_ENABLE(verification-options,
  [AS_HELP_STRING([--enable-verification-options],
                [Build simulator with verification options])],
  [SIM_MODE="-DVERIFICATION_OPTIONS $SIM_MODE";
   AC_MSG_NOTICE([Verification options enabled.])]
)

AC_ARG_ENABLE(io-devices,
  [AS_HELP_STRING([--enable-io-devices],
                [Enable memory mapped devices])],
  [enable_io="yes";
   SIM_MODE="-DENABLE_IO_DEVICES $SIM_MODE";
   EXTRA_SRCS="mem/mmap/IODeviceIrq.cpp mem/mmap/IODeviceKeyboard.cpp mem/mmap/IODeviceScreen.cpp mem/mmap/IODeviceSound.cpp \
               $EXTRA_SRCS";
   EXTRA_INCDIRS="$(pkg-config --cflags gdk-2.0 gtkglext-1.0) \
                  $EXTRA_INCDIRS";
   EXTRA_LIBS="$(pkg-config --libs-only-l gdk-2.0 gtkglext-1.0) \
               $EXTRA_LIBS";
   EXTRA_LIBDIRS="$(pkg-config --libs-only-L gdk-2.0 gtkglext-1.0)\
                  $EXTRA_LIBDIRS";
   AC_MSG_NOTICE([Memory mapped evices enabled.])]
)

dnl #----------------------------------------------------------------
dnl # External Software
dnl # AC_ARG_WITH (package, help-string, [action-if-given], [action-if-not-given])
dnl #----------------------------------------------------------------

dnl ----------------------------------------------------------------
dnl  ELFIO location: --with-elfio
dnl ----------------------------------------------------------------
AC_ARG_WITH(elfio,
  [AS_HELP_STRING([--with-elfio=PATH],
                  [Path to ELFIO installation])],
  # action if given
  [ELFIO="$with_elfio"],
  # action if not given
  [ELFIO="/group/project/arc_tools/arcsim/sys/$ARCH/elfio/1.0.3"]
)
AC_MSG_NOTICE([ELFIO location: '$ELFIO'])

dnl ----------------------------------------------------------------
dnl  ELFIO location: --with-llvm
dnl ----------------------------------------------------------------
AC_ARG_WITH(llvm,
  [AS_HELP_STRING([--with-llvm=PATH],
                  [Path to LLVM installation])],
  # action if given
  [LLVM="$with_llvm"],
  # action if not given
  [LLVM="/group/project/arc_tools/arcsim/sys/$ARCH/llvm/RELEASE_29"]
)
AC_MSG_NOTICE([LLVM location: '$LLVM'])

dnl ----------------------------------------------------------------
dnl  MetaWare location: --with-mw
dnl ----------------------------------------------------------------
AC_ARG_WITH(mw,
  [AS_HELP_STRING([--with-mw=PATH],
                  [Path to MetaWare installation])],
  # action if given
  [ METAWARE="$with_mw";
    EXTRA_SRCS="api/arc/api_arcint.cpp api/arc/prof_count.cpp $EXTRA_SRCS";
    AC_MSG_NOTICE([METAWARE location: '$METAWARE']);
    AC_MSG_NOTICE([METAWARE debugger interface enabled.])],
  # action if not given
  [true]
)


dnl ----------------------------------------------------------------
dnl Pre-Processor Flags
dnl ----------------------------------------------------------------

CPPFLAGS="-DHAVE_MMX -DHAVE_SSE2 -D_GNU_SOURCE -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS"


dnl ----------------------------------------------------------------
dnl Set the compiler flags and target architecture.
dnl ----------------------------------------------------------------

AR_TOOL="ar cru"

case "$target" in
    *linux*)
        case "$CXX" in
            clang++ | c++ | g++)
                TARGET_ARCH="linux"
                CXXWARNINGS="-Wswitch -Wreturn-type -Wparentheses -Wconversion -Wsign-compare  -Wno-format -Wunused-function -Wunused-label -Wunused-value -Wunused-variable -Wno-deprecated-declarations -Wno-invalid-offsetof"
                CXXFLAGS="-O2 $MACHINE -mmmx -msse2 -fPIC -fomit-frame-pointer -fno-exceptions -fno-rtti -pipe $CXXFLAGS $CPPFLAGS"
                CFLAGS="-O2 $MACHINE -mmmx -msse2 -fPIC -pipe $CFLAGS $CPPFLAGS"
                LDFLAGS="$MACHINE  -rdynamic "
                SHARED_CCFLAGS="$MACHINE -shared -fPIC"
                SHARED_LDFLAGS="$MACHINE -shared -fPIC --allow-shlib-undefined" 
                EXTRA_LIBS="$EXTRA_LIBS -lc -ldl"
                AS_IF([test "x$enable_io" = "xyes"], [
                  EXTRA_LIBS="$EXTRA_LIBS -lopenal"
                ])
                ;;
            *)
                AC_MSG_ERROR("sorry...'$CXX' compiler not supported")
                ;;
        esac
        ;;
    *darwin*)
        case "$CXX" in
            clang++ | c++ | g++)
                TARGET_ARCH="macosx"
                CXXWARNINGS="-Wswitch -Wreturn-type -Wparentheses -Wconversion -Wsign-compare  -Wno-format -Wunused-function -Wunused-label -Wunused-value -Wunused-variable -Wno-deprecated-declarations -Wno-invalid-offsetof"
                CXXFLAGS="-O2 $MACHINE -mmmx -msse2 -fPIC -fomit-frame-pointer -fno-exceptions -fno-rtti -pipe $CXXFLAGS $CPPFLAGS"
                CFLAGS="-O2 $MACHINE -mmmx -msse2 -fPIC -pipe $CFLAGS $CPPFLAGS"
                LDFLAGS="-arch $ARCH  -rdynamic "
                SHARED_CCFLAGS="$MACHINE -dynamiclib -flat_namespace -undefined suppress"
                SHARED_LDFLAGS="-arch $ARCH -dylib -flat_namespace -undefined suppress"
                EXTRA_LIBS="$EXTRA_LIBS -lc -ldl"
                AS_IF([test "x$enable_io" = "xyes"], [
                  EXTRA_INCDIRS="$EXTRA_INCDIRS -FOpenAL"
                  EXTRA_LIBS="$EXTRA_LIBS -framework OpenAL"
                ])
                ;;
            *)
                AC_MSG_ERROR("sorry...'$CXX' compiler not supported")
                ;;
        esac
        ;;
    *cygwin*)
        case "$CXX" in
            clang++ | c++ | g++)
                TARGET_ARCH="cygwin"
                CXXWARNINGS="-Wswitch -Wreturn-type -Wparentheses -Wconversion -Wsign-compare  -Wno-format -Wunused-function -Wunused-label -Wunused-value -Wunused-variable -Wno-deprecated-declarations -Wno-invalid-offsetof"
                CXXFLAGS="-O2 $MACHINE -mmmx -msse2 -fPIC -fomit-frame-pointer -fno-exceptions -fno-rtti -pipe $CXXFLAGS $CPPFLAGS"
                CFLAGS="-O2 $MACHINE -mmmx -msse2 -fPIC -pipe $CFLAGS $CPPFLAGS"
                LDFLAGS="$MACHINE "
                SHARED_CCFLAGS="$MACHINE -shared -fPIC"
                SHARED_LDFLAGS="$MACHINE -shared -fPIC --allow-shlib-undefined"
                ;;
            *)
                AC_MSG_ERROR("sorry...'$CXX' compiler not supported")
                ;;
        esac
        ;;
    *)
        AC_MSG_ERROR("sorry...'$target' architecture not supported")
        ;;
esac


dnl ----------------------------------------------------------------
dnl Set the default prefix and make sure the prefix is absolute.
dnl ----------------------------------------------------------------

test "x$prefix" = xNONE && prefix=$srcdir
current_dir=`pwd`; cd $prefix; prefix=`pwd`; cd $current_dir

dnl ----------------------------------------------------------------
dnl Set some defines - these are injected into src/config.in by autoheader
dnl ----------------------------------------------------------------

AC_DEFINE_UNQUOTED([JIT_CC],
          ["gcc"],
          [Compiler used for JIT compilation.])
AC_DEFINE_UNQUOTED([JIT_CCOPT],
          ["-pipe $SHARED_CCFLAGS"],
          [Compiler option used for JIT compilation.])

AC_DEFINE_UNQUOTED([DEFAULT_SYS_ARCH_FILE],
          ["$current_dir/etc/encore.arc"],
          [Simulator Architecture file.])
AC_DEFINE_UNQUOTED([DEFAULT_ISA_FILE],
          ["$current_dir/etc/encore.isa"],
          [Simulator ISA file.])

dnl ----------------------------------------------------------------
dnl Substitution variables.
dnl ----------------------------------------------------------------

AC_SUBST(CXX)
AC_SUBST(CXXFLAGS)
AC_SUBST(CXXWARNINGS)

AC_SUBST(CC)
AC_SUBST(CFLAGS)
AC_SUBST(SHARED_CCFLAGS)

AC_SUBST(LDFLAGS)
AC_SUBST(SHARED_LDFLAGS)

AC_SUBST(LLVM_VERSION)
AC_SUBST(SIM_MODE)
AC_SUBST(TARGET_ARCH)
AC_SUBST(AR_TOOL)

AC_SUBST(ELFIO)
AC_SUBST(LLVM)
AC_SUBST(METAWARE)
AC_SUBST(EXTRA_SRCS)
AC_SUBST(EXTRA_INCDIRS)
AC_SUBST(EXTRA_LIBS)
AC_SUBST(EXTRA_LIBDIRS)

dnl ----------------------------------------------------------------
dnl Create Makefiles
dnl ----------------------------------------------------------------

AC_OUTPUT(
  Makefile
  src/Makefile
  examples/eia/Makefile
  examples/eia/run.sh
  examples/mem/Makefile
  tests/regression/mk/config.mk
  tests/api/Makefile
) 

dnl #----------------------------------------------------------------
dnl # ArcSim configuration END
dnl #----------------------------------------------------------------
