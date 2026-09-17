/* Build an unmodified, x86-only LLVM/Clang for the compile-speed experiment.
   LLVM source and every generated file stay below build/. The installed
   compiler remains untouched. Invoke explicitly with -RunExternal.
   No path is pinned to one machine: every tool comes from the command line, or
   from CPC_CLANG/CPC_NINJA/CPC_RC/CPC_MT, or from PATH under its own name. */
#include "common/native_tool.h"

/* A bare tool name is resolved through PATH by the child process, so the
   presence check only applies when the caller gave an explicit path. */
static int tool_present(const char *tool) {
    if (strchr(tool, '\\') || strchr(tool, '/')) return nt_exists(tool);
    return 1;
}

int main(int argc, char **argv) {
    char scripts[NT_PATH], root[NT_PATH], source[NT_PATH], output[NT_PATH];
    char compiler[NT_PATH], ninja[NT_PATH], rc[NT_PATH], mt[NT_PATH];
    char cflag[NT_PATH+64], cppflag[NT_PATH+64], nflag[NT_PATH+64], rcflag[NT_PATH+64], mtflag[NT_PATH+64];
    const char *env;
    const char *args[64]; int i, n=0, authorized=0, configure_only=0;
    NtProcessResult r;
    nt_module_directory(scripts, sizeof scripts); nt_tree_root(root, sizeof root);
    nt_join(source, sizeof source, root, "src/build/clang-competitive/llvm-source/llvm");
    nt_join(output, sizeof output, root, "src/build/clang-competitive/llvm-minimal");
    env = getenv("CPC_CLANG"); snprintf(compiler, sizeof compiler, "%s", env && *env ? env : "clang.exe");
    env = getenv("CPC_NINJA"); snprintf(ninja, sizeof ninja, "%s", env && *env ? env : "ninja.exe");
    rc[0] = mt[0] = 0;
    env = getenv("CPC_RC"); if (env && *env) snprintf(rc, sizeof rc, "%s", env);
    env = getenv("CPC_MT"); if (env && *env) snprintf(mt, sizeof mt, "%s", env);
    for (i=1; i<argc; ++i) {
        if (!strcmp(argv[i], "-RunExternal")) authorized=1;
        else if (!strcmp(argv[i], "-ConfigureOnly")) configure_only=1;
        else if (!strcmp(argv[i], "-Source") && i+1<argc) snprintf(source,sizeof source,"%s",argv[++i]);
        else if (!strcmp(argv[i], "-Out") && i+1<argc) snprintf(output,sizeof output,"%s",argv[++i]);
        else if (!strcmp(argv[i], "-Clang") && i+1<argc) snprintf(compiler,sizeof compiler,"%s",argv[++i]);
        else if (!strcmp(argv[i], "-Ninja") && i+1<argc) snprintf(ninja,sizeof ninja,"%s",argv[++i]);
        else if (!strcmp(argv[i], "-Rc") && i+1<argc) snprintf(rc,sizeof rc,"%s",argv[++i]);
        else if (!strcmp(argv[i], "-Mt") && i+1<argc) snprintf(mt,sizeof mt,"%s",argv[++i]);
        else { fputs("build-clang-minimal.exe -RunExternal [-ConfigureOnly] [-Source LLVM_DIR] [-Out DIR] [-Clang EXE] [-Ninja EXE] [-Rc EXE] [-Mt EXE]\n"
                     "Tools default to PATH names or CPC_CLANG/CPC_NINJA/CPC_RC/CPC_MT; -Rc/-Mt are optional and CMake finds them otherwise.\n",stderr); return 2; }
    }
    if (!authorized) { fputs("Clang source builds require -RunExternal\n",stderr); return 2; }
    if (!nt_exists(source) || !tool_present(compiler) || !tool_present(ninja)) nt_die("missing source or build tool",NULL);
    snprintf(cflag,sizeof cflag,"-DCMAKE_C_COMPILER=%s",compiler);
    snprintf(cppflag,sizeof cppflag,"-DCMAKE_CXX_COMPILER=%s",compiler);
    snprintf(nflag,sizeof nflag,"-DCMAKE_MAKE_PROGRAM=%s",ninja);
#define A(x) args[n++]=(x)
    A("cmake.exe"); A("-S"); A(source); A("-B"); A(output); A("-G"); A("Ninja");
    A(cflag); A(cppflag); A(nflag);
    if (*rc) { snprintf(rcflag,sizeof rcflag,"-DCMAKE_RC_COMPILER=%s",rc); A(rcflag); }
    if (*mt) { snprintf(mtflag,sizeof mtflag,"-DCMAKE_MT=%s",mt); A(mtflag); }
    A("-DCMAKE_BUILD_TYPE=MinSizeRel");
    A("-DLLVM_ENABLE_PROJECTS=clang"); A("-DLLVM_TARGETS_TO_BUILD=X86");
    A("-DLLVM_TARGET_ARCH=X86"); A("-DLLVM_DEFAULT_TARGET_TRIPLE=x86_64-pc-windows-msvc");
    A("-DLLVM_ENABLE_ASSERTIONS=OFF"); A("-DLLVM_ENABLE_EH=OFF"); A("-DLLVM_ENABLE_RTTI=OFF");
    A("-DLLVM_ENABLE_THREADS=OFF"); A("-DLLVM_ENABLE_UNWIND_TABLES=OFF");
    A("-DLLVM_ENABLE_BACKTRACES=OFF"); A("-DLLVM_ENABLE_CRASH_OVERRIDES=OFF");
    A("-DLLVM_ENABLE_RPMALLOC=ON"); A("-DLLVM_ENABLE_ZLIB=OFF"); A("-DLLVM_ENABLE_ZSTD=OFF");
    A("-DLLVM_ENABLE_LIBXML2=OFF"); A("-DLLVM_ENABLE_LIBEDIT=OFF"); A("-DLLVM_ENABLE_LIBPFM=OFF");
    A("-DLLVM_INCLUDE_TESTS=OFF"); A("-DLLVM_INCLUDE_BENCHMARKS=OFF");
    A("-DLLVM_INCLUDE_EXAMPLES=OFF"); A("-DLLVM_INCLUDE_DOCS=OFF");
    A("-DCLANG_INCLUDE_TESTS=OFF"); A("-DCLANG_INCLUDE_DOCS=OFF"); A("-DCLANG_INCLUDE_EXAMPLES=OFF");
    A("-DCLANG_ENABLE_STATIC_ANALYZER=OFF"); A("-DCLANG_ENABLE_OBJC_REWRITER=OFF");
    A("-DLLVM_ENABLE_LTO=OFF"); A("-DLLVM_USE_LINKER=lld");
    A("-DBUILD_SHARED_LIBS=OFF"); A("-DLLVM_BUILD_LLVM_DYLIB=OFF"); A("-DCLANG_LINK_CLANG_DYLIB=OFF");
    A("-DCMAKE_EXE_LINKER_FLAGS=-Wl,/threads:1,/debug:none,/opt:ref,/opt:icf");
    A("-DLLVM_PARALLEL_COMPILE_JOBS=1"); A("-DLLVM_PARALLEL_LINK_JOBS=1");
    A("-DLLVM_PARALLEL_TABLEGEN_JOBS=1");
    args[n]=NULL;
    r=nt_run(args,root,600000,1);
    if (!r.started || r.timed_out || r.exit_code) { nt_process_free(&r); return 1; }
    nt_process_free(&r);
    if (configure_only) return 0;
    n=0; A("cmake.exe"); A("--build"); A(output); A("--target"); A("clang"); A("--parallel"); A("1"); args[n]=NULL;
    r=nt_run(args,root,0,1);
    printf("Serial Clang source build: %.3fs, exit %lu\n",r.wall_seconds,(unsigned long)r.exit_code);
    i=(!r.started || r.timed_out || r.exit_code) ? 1 : 0; nt_process_free(&r); return i;
#undef A
}
