#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$ROOT_DIR/build"
SHIM_DIR="$ROOT_DIR/shim"
DIST_DIR="$ROOT_DIR/dist"
WASI_TERMIOS_DIR="${WASI_TERMIOS_DIR:-$ROOT_DIR/deps/wasi-termios-brsh}"

WASI_SDK_VERSION="25"
WASI_SDK_FULL="25.0"
BINARYEN_VERSION="119"
NCURSES_VERSION="6.5"
NANO_VERSION="8.0"

mkdir -p "$BUILD_DIR" "$DIST_DIR" "$SHIM_DIR/lib"
cd "$BUILD_DIR"

if [[ ! -f "$WASI_TERMIOS_DIR/src/termios.c" ]] && [[ -f "$ROOT_DIR/../wasi-termios-brsh/src/termios.c" ]]; then
    WASI_TERMIOS_DIR="$ROOT_DIR/../wasi-termios-brsh"
fi
if [[ ! -f "$WASI_TERMIOS_DIR/src/termios.c" ]]; then
    echo "=== Initializing wasi-termios-brsh ==="
    git -C "$ROOT_DIR" submodule update --init --recursive 2>/dev/null || \
    git clone https://github.com/eliotstocker/wasi-termios-brsh.git "$WASI_TERMIOS_DIR"
fi

# Detect host OS / arch for WASI SDK
OS="$(uname -s)"
ARCH="$(uname -m)"
if [[ "$OS" == "Darwin" ]]; then
    WASI_OS="macos"
    BUILD_TRIPLET="${ARCH}-apple-darwin"
elif [[ "$OS" == "Linux" ]]; then
    WASI_OS="linux"
    BUILD_TRIPLET="${ARCH}-pc-linux-gnu"
else
    echo "Unsupported host OS: $OS" >&2
    exit 1
fi
if [[ "$ARCH" == "x86_64" ]]; then
    WASI_ARCH="x86_64"
else
    WASI_ARCH="arm64"
fi

WASI_SDK_DIR="$BUILD_DIR/wasi-sdk-${WASI_SDK_FULL}-${WASI_ARCH}-${WASI_OS}"
if [[ ! -d "$WASI_SDK_DIR" ]]; then
    echo "=== Downloading WASI SDK ${WASI_SDK_FULL} ==="
    curl -L "https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-${WASI_SDK_VERSION}/wasi-sdk-${WASI_SDK_FULL}-${WASI_ARCH}-${WASI_OS}.tar.gz" -o wasi-sdk.tar.gz
    tar -xzf wasi-sdk.tar.gz
fi

BINARYEN_DIR="$BUILD_DIR/binaryen-version_${BINARYEN_VERSION}"
if command -v wasm-opt &> /dev/null; then
    WASM_OPT="$(command -v wasm-opt)"
    echo "=== Using system wasm-opt: $WASM_OPT ==="
else
    WASM_OPT="$BINARYEN_DIR/bin/wasm-opt"
    if [[ ! -x "$WASM_OPT" ]]; then
        echo "=== Downloading Binaryen version_${BINARYEN_VERSION} ==="
        curl -L "https://github.com/WebAssembly/binaryen/releases/download/version_${BINARYEN_VERSION}/binaryen-version_${BINARYEN_VERSION}-${WASI_ARCH}-${WASI_OS}.tar.gz" -o binaryen.tar.gz
        tar -xzf binaryen.tar.gz
    fi
fi

if [[ ! -d "$BUILD_DIR/ncurses-${NCURSES_VERSION}" ]]; then
    echo "=== Downloading ncurses ${NCURSES_VERSION} ==="
    curl -L "https://ftp.gnu.org/gnu/ncurses/ncurses-${NCURSES_VERSION}.tar.gz" -o ncurses.tar.gz
    tar -xzf ncurses.tar.gz
fi

if [[ ! -d "$BUILD_DIR/nano-${NANO_VERSION}" ]]; then
    echo "=== Downloading GNU nano ${NANO_VERSION} ==="
    curl -L "https://ftp.gnu.org/gnu/nano/nano-${NANO_VERSION}.tar.xz" -o nano.tar.xz
    tar -xJf nano.tar.xz
fi

NCURSES_SRC="$BUILD_DIR/ncurses-${NCURSES_VERSION}"
NANO_SRC="$BUILD_DIR/nano-${NANO_VERSION}"
HOST_NCURSES="$BUILD_DIR/host-ncurses"
WASM_NCURSES="$BUILD_DIR/wasm-ncurses"
WASM_NANO="$BUILD_DIR/wasm-nano"
DEPS_DIR="$BUILD_DIR/deps"
TERMINFO_DIR="$BUILD_DIR/terminfo-minimal"

# 1. Build host tic & infocmp
if [[ ! -x "$HOST_NCURSES/progs/tic" ]]; then
    echo "=== Building host tic and infocmp ==="
    mkdir -p "$HOST_NCURSES"
    (
        cd "$HOST_NCURSES"
        "$NCURSES_SRC/configure" \
            --without-cxx \
            --without-cxx-binding \
            --without-ada \
            --without-tests \
            --without-manpages \
            --without-debug
        make -C include -j4
        make -C ncurses -j4
        make -C progs tic infocmp -j4
    )
fi

export PATH="$HOST_NCURSES/progs:$WASI_SDK_DIR/bin:$PATH"

# 2. Build WASI POSIX / termios shim
echo "=== Compiling wasi-termios-brsh and WASI POSIX shim ==="
"$WASI_SDK_DIR/bin/clang" --target=wasm32-wasip1 -O2 \
    -I"$WASI_TERMIOS_DIR/include" \
    -c "$WASI_TERMIOS_DIR/src/termios.c" -o "$SHIM_DIR/lib/termios.o"
"$WASI_SDK_DIR/bin/clang" --target=wasm32-wasip1 -O2 \
    -I"$WASI_TERMIOS_DIR/include" -I"$SHIM_DIR/include" \
    -D_WASI_EMULATED_SIGNAL -D_WASI_EMULATED_GETPID -D_WASI_EMULATED_PROCESS_CLOCKS \
    -c "$SHIM_DIR/src/shim.c" -o "$SHIM_DIR/lib/shim.o"
"$WASI_SDK_DIR/bin/llvm-ar" rcs "$SHIM_DIR/lib/libwasi_posix_shim.a" "$SHIM_DIR/lib/termios.o" "$SHIM_DIR/lib/shim.o"

# 3. Compile minimal terminfo DB & cross-compile ncurses 6.5
echo "=== Compiling minimal terminfo DB ==="
rm -rf "$TERMINFO_DIR"
mkdir -p "$TERMINFO_DIR"
"$HOST_NCURSES/progs/tic" -x -o "$TERMINFO_DIR" \
    -e xterm,xterm-256color,vt100,vt220,dumb,ansi,linux \
    "$NCURSES_SRC/misc/terminfo.src"

echo "=== Cross-compiling ncurses ${NCURSES_VERSION} for wasm32-wasip1 ==="
rm -rf "$WASM_NCURSES" "$DEPS_DIR"
mkdir -p "$WASM_NCURSES" "$DEPS_DIR"
(
    cd "$WASM_NCURSES"
    COMMON_FLAGS="--target=wasm32-wasip1 -O2 -I$WASI_TERMIOS_DIR/include -I$SHIM_DIR/include -D_WASI_EMULATED_SIGNAL -D_WASI_EMULATED_GETPID -D_WASI_EMULATED_PROCESS_CLOCKS -D_WASI_EMULATED_MMAN"
    COMMON_LDFLAGS="-L$SHIM_DIR/lib -lwasi_posix_shim -lwasi-emulated-signal -lwasi-emulated-getpid -lwasi-emulated-process-clocks -lwasi-emulated-mman"

    CC="$WASI_SDK_DIR/bin/clang" \
    AR="$WASI_SDK_DIR/bin/llvm-ar" \
    RANLIB="$WASI_SDK_DIR/bin/llvm-ranlib" \
    NM="$WASI_SDK_DIR/bin/llvm-nm" \
    STRIP="$WASI_SDK_DIR/bin/llvm-strip" \
    CFLAGS="$COMMON_FLAGS" \
    LDFLAGS="$COMMON_LDFLAGS" \
    cf_cv_func_nanosleep=yes \
    cf_cv_func_mkstemp=yes \
    cf_cv_working_poll=yes \
    ac_cv_func_sigaction=yes \
    ac_cv_func_sigvec=no \
    ac_cv_header_termios_h=yes \
    ac_cv_header_sys_termios_h=yes \
    ac_cv_header_sys_ioctl_h=yes \
    "$NCURSES_SRC/configure" \
        --host=wasm32-unknown-wasi \
        --build="${BUILD_TRIPLET}" \
        --prefix="$DEPS_DIR" \
        --without-cxx \
        --without-cxx-binding \
        --without-ada \
        --without-tests \
        --without-progs \
        --without-manpages \
        --with-termlib \
        --without-debug \
        --without-profile \
        --without-shared \
        --with-normal \
        --disable-db-install \
        --with-fallbacks="xterm-256color,xterm,vt220,vt100,dumb" \
        --with-tic-path="$HOST_NCURSES/progs/tic" \
        --with-infocmp-path="$HOST_NCURSES/progs/infocmp" \
        --enable-widec \
        --disable-stripping

    sh "$NCURSES_SRC/ncurses/tinfo/MKfallback.sh" \
        "$TERMINFO_DIR" \
        "$NCURSES_SRC/misc/terminfo.src" \
        "$HOST_NCURSES/progs/tic" \
        "$HOST_NCURSES/progs/infocmp" \
        xterm-256color xterm vt220 vt100 dumb \
        > "$WASM_NCURSES/ncurses/fallback.c"

    make -C include -j4
    make -C ncurses -j4
    make -C panel -j4
    make -C menu -j4
    make -C form -j4

    make -C include install
    make -C ncurses install
    make -C panel install
    make -C menu install
    make -C form install

    for h in "$DEPS_DIR/include/ncursesw/"*.h; do
        ln -sf "ncursesw/$(basename "$h")" "$DEPS_DIR/include/$(basename "$h")"
    done
    ln -sf libncursesw.a "$DEPS_DIR/lib/libncurses.a"
    ln -sf libtinfow.a "$DEPS_DIR/lib/libtinfo.a"
)

# 4. Cross-compile GNU nano 8.0
echo "=== Cross-compiling GNU nano ${NANO_VERSION} for wasm32-wasip1 ==="
rm -rf "$WASM_NANO"
mkdir -p "$WASM_NANO"
(
    cd "$WASM_NANO"
    COMMON_FLAGS="--target=wasm32-wasip1 -O2 -I$WASI_TERMIOS_DIR/include -I$SHIM_DIR/include -I$DEPS_DIR/include -I$DEPS_DIR/include/ncursesw -D_WASI_EMULATED_SIGNAL -D_WASI_EMULATED_GETPID -D_WASI_EMULATED_PROCESS_CLOCKS -D_WASI_EMULATED_MMAN"
    COMMON_LDFLAGS="-Wl,--export=set_terminal_size -Wl,--export=get_termios_lflag -Wl,--wrap=poll -Wl,--wrap=isatty -Wl,--wrap=readlink -Wl,--wrap=readlinkat -L$DEPS_DIR/lib -L$SHIM_DIR/lib -lncursesw -ltinfow -lwasi_posix_shim -lwasi-emulated-signal -lwasi-emulated-getpid -lwasi-emulated-process-clocks -lwasi-emulated-mman"

    CC="$WASI_SDK_DIR/bin/clang" \
    AR="$WASI_SDK_DIR/bin/llvm-ar" \
    RANLIB="$WASI_SDK_DIR/bin/llvm-ranlib" \
    NM="$WASI_SDK_DIR/bin/llvm-nm" \
    STRIP="$WASI_SDK_DIR/bin/llvm-strip" \
    CFLAGS="$COMMON_FLAGS" \
    CPPFLAGS="-I$WASI_TERMIOS_DIR/include -I$SHIM_DIR/include -I$DEPS_DIR/include -I$DEPS_DIR/include/ncursesw -D_WASI_EMULATED_SIGNAL -D_WASI_EMULATED_GETPID -D_WASI_EMULATED_PROCESS_CLOCKS -D_WASI_EMULATED_MMAN" \
    LDFLAGS="$COMMON_LDFLAGS" \
    LIBS="-lncursesw -ltinfow -lwasi_posix_shim -lwasi-emulated-signal -lwasi-emulated-getpid -lwasi-emulated-process-clocks -lwasi-emulated-mman" \
    NCURSESW_CFLAGS="-I$DEPS_DIR/include/ncursesw -I$DEPS_DIR/include" \
    NCURSESW_LIBS="-L$DEPS_DIR/lib -lncursesw -ltinfow" \
    ac_cv_header_termios_h=yes \
    ac_cv_header_pwd_h=yes \
    ac_cv_header_sys_wait_h=yes \
    ac_cv_header_sys_ioctl_h=yes \
    ac_cv_header_ncursesw_ncurses_h=yes \
    ac_cv_header_ncurses_h=yes \
    ac_cv_header_curses_h=yes \
    ac_cv_lib_ncursesw_initscr=yes \
    ac_cv_lib_ncursesw_waddwstr=yes \
    ac_cv_search_initscr="-lncursesw -ltinfow" \
    ac_cv_func_getpwuid=yes \
    ac_cv_func_getpwnam=yes \
    ac_cv_func_chmod=yes \
    ac_cv_func_chown=yes \
    ac_cv_func_fchmod=yes \
    ac_cv_func_fchown=yes \
    ac_cv_func_fork=no \
    ac_cv_func_vfork=no \
    ac_cv_func_pipe=yes \
    ac_cv_func_wait=no \
    ac_cv_func_waitpid=no \
    ac_cv_func_sigaction=yes \
    ac_cv_func_kill=yes \
    ac_cv_func_use_default_colors=yes \
    ac_cv_func_set_escdelay=yes \
    ac_cv_func_key_defined=yes \
    gl_cv_func_working_mktime=yes \
    gl_cv_func_MBRTOWC_EMPTY_INPUT=yes \
    gl_cv_func_mbrtowc_empty_input=yes \
    gl_cv_func_mbrtowc_incomplete_state=yes \
    gl_cv_func_mbrtowc_sanitycheck=yes \
    gl_cv_func_mbrtowc_null_arg1=yes \
    gl_cv_func_mbrtowc_null_arg2=yes \
    gl_cv_func_mbrtowc_retval=yes \
    gl_cv_func_mbrtowc_nul_retval=yes \
    gl_cv_func_mbrtowc_stores_nuls=no \
    gl_cv_func_btowc_nul=yes \
    gl_cv_func_wcrtomb_retval=yes \
    gl_cv_func_iswcntrl_works=yes \
    gl_cv_func_wcwidth_works=yes \
    gl_cv_func_re_compile_pattern_working=no \
    gl_cv_func_fnmatch_posix=yes \
    gl_cv_func_gettimeofday_clobber=no \
    gl_cv_func_memchr_works=yes \
    gl_cv_func_stat_file_slash=yes \
    gl_cv_func_lstat_dereferences_slashed_symlink=yes \
    gl_cv_func_FDOPENDIR_WORKS=yes \
    gl_cv_func_fdopendir_works=yes \
    gl_cv_func_fstatat_zero_flag=yes \
    gl_cv_func_open_directory_works=yes \
    gl_cv_func_perror_works=yes \
    gl_cv_func_printf_sizes_c99=yes \
    gl_cv_func_printf_long_double=yes \
    gl_cv_func_printf_infinite=yes \
    gl_cv_func_printf_infinite_long_double=yes \
    gl_cv_func_printf_directive_a=yes \
    gl_cv_func_printf_directive_f=yes \
    gl_cv_func_printf_directive_n=no \
    gl_cv_func_printf_directive_ls=yes \
    gl_cv_func_printf_positions=yes \
    gl_cv_func_printf_flag_grouping=yes \
    gl_cv_func_printf_flag_leftadjust=yes \
    gl_cv_func_printf_flag_zero=yes \
    gl_cv_func_printf_precision=yes \
    gl_cv_func_printf_enomem=yes \
    gl_cv_func_snprintf_truncation_c99=yes \
    gl_cv_func_snprintf_retval_c99=yes \
    gl_cv_func_snprintf_directive_n=no \
    gl_cv_func_snprintf_size1=yes \
    gl_cv_func_vsnprintf_zerosize_c99=yes \
    gl_cv_func_getcwd_null=yes \
    gl_cv_func_getcwd_path_max=yes \
    gl_cv_func_getcwd_abort_bug=no \
    gl_cv_func_duplocale_works=yes \
    gl_cv_func_working_getdelim=yes \
    gl_cv_func_working_getline=yes \
    gl_cv_func_getopt_posix=yes \
    gl_cv_func_getopt_long_gnu=yes \
    gl_cv_func_canonicalize_works=yes \
    gl_cv_func_realpath_works=yes \
    gl_cv_func_futimens_works=yes \
    gl_cv_func_utimensat_works=yes \
    gl_cv_func_sleep_works=yes \
    gl_cv_func_strerror_0_works=yes \
    gl_cv_func_strcasestr_linear=yes \
    gl_cv_func_strstr_linear=yes \
    "$NANO_SRC/configure" \
        --host=wasm32-unknown-wasi \
        --build="${BUILD_TRIPLET}" \
        --prefix=/usr \
        --sysconfdir=/etc \
        --disable-nls \
        --disable-speller \
        --disable-libmagic \
        --enable-utf8

    make -j4
    "$WASI_SDK_DIR/bin/llvm-strip" src/nano -o "$DIST_DIR/nano.wasm"

    echo "=== Running wasm-opt with Asyncify ==="
    "$WASM_OPT" "$DIST_DIR/nano.wasm" -o "$DIST_DIR/nano.wasm" -O2 \
        --asyncify \
        --pass-arg=asyncify-imports@wasi_snapshot_preview1.fd_read,wasi_snapshot_preview1.poll_oneoff

    cp "$DIST_DIR/nano.wasm" "$ROOT_DIR/nano.wasm"
)

echo "=== Build complete: $DIST_DIR/nano.wasm ==="
ls -lh "$DIST_DIR/nano.wasm"
