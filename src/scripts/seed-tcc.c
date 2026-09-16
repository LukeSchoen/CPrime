#include "common/native_tool.h"

static int run_checked(const char *label, const char *const *args, const char *root,
                       unsigned timeout, double *seconds) {
    NtProcessResult result = nt_run(args, root, timeout, 1);
    int failed = !result.started || result.timed_out || result.exit_code;
    if (seconds) *seconds = result.wall_seconds;
    if (failed) fprintf(stderr, "ERROR: %s failed (exit %lu)\n", label,
                        (unsigned long)result.exit_code);
    nt_process_free(&result); return !failed;
}

static int compiler_arguments(const char **args, const char *compiler,
                              const char *root, const char *runtime, const char *output) {
    static char inc1[NT_PATH], inc2[NT_PATH], inc3[NT_PATH], inc4[NT_PATH],
                runtime_inc[NT_PATH], sdk_inc[NT_PATH], winapi_inc[NT_PATH],
                source[NT_PATH], bflag[NT_PATH+3];
    int n = 0;
    snprintf(inc1,sizeof inc1,"-I%s\\src\\include\\cprime",root);
    snprintf(inc2,sizeof inc2,"-I%s\\src\\compiler\\frontend",root);
    snprintf(inc3,sizeof inc3,"-I%s\\src\\compiler\\middleend",root);
    snprintf(inc4,sizeof inc4,"-I%s\\src\\compiler\\backend\\x64",root);
    snprintf(runtime_inc,sizeof runtime_inc,"-I%s\\src\\include\\runtime",root);
    snprintf(sdk_inc,sizeof sdk_inc,"-I%s\\src\\third-party\\win32-sdk\\include",root);
    snprintf(winapi_inc,sizeof winapi_inc,"-I%s\\src\\third-party\\win32-sdk\\include\\winapi",root);
    nt_join(source,sizeof source,root,"src\\compiler\\driver\\cprime.c");
    args[n++]=compiler; args[n++]="-O2";
    if(runtime) { snprintf(bflag,sizeof bflag,"-B%s",runtime); args[n++]=bflag;
        args[n++]=runtime_inc; args[n++]=sdk_inc; args[n++]=winapi_inc; }
    args[n++]=inc1; args[n++]=inc2; args[n++]=inc3; args[n++]=inc4;
    args[n++]="-I."; args[n++]="-DCPRIME_TARGET_PE"; args[n++]="-DCPRIME_TARGET_X86_64";
    args[n++]="-Dopen=_open"; args[n++]="-Dread=_read"; args[n++]="-Dclose=_close";
    args[n++]="-Dlseek=_lseek"; args[n++]="-Dunlink=_unlink"; args[n++]="-Dfdopen=_fdopen";
    args[n++]="-Dgetcwd=_getcwd"; args[n++]="-Dstricmp=_stricmp";
    args[n++]="-Dstrnicmp=_strnicmp"; args[n++]="-Dstrlwr=_strlwr";
    args[n++]=source; args[n++]="-o"; args[n++]=output; args[n]=NULL;
    return n;
}

int main(int argc, char **argv) {
    char scripts[NT_PATH], root[NT_PATH], tcc[NT_PATH], out[NT_PATH], seed[NT_PATH], self[NT_PATH],
         test_source[NT_PATH], test_exe[NT_PATH], bflag[NT_PATH + 3];
    const char *compile[32], *test[8], *run[3];
    int authorized=0, i; double seed_seconds=0, self_seconds=0, test_seconds=0;
    nt_module_directory(scripts,sizeof scripts); nt_tree_root(root,sizeof root);
    nt_join(tcc,sizeof tcc,root,"src\\third-party\\tcc\\win32\\tcc.exe");
    nt_join(out,sizeof out,root,"src\\build\\seed-tcc");
    for(i=1;i<argc;i++) {
        if(!_stricmp(argv[i],"-RunExternal")) authorized=1;
        else if(!_stricmp(argv[i],"-TccPath")&&i+1<argc) strcpy(tcc,argv[++i]);
        else if(!_stricmp(argv[i],"-OutDir")&&i+1<argc) strcpy(out,argv[++i]);
        else if(!_stricmp(argv[i],"-Help")||!_stricmp(argv[i],"--help")) {
            puts("seed-tcc.exe -RunExternal [-TccPath PATH] [-OutDir DIR]\nBuilds the C-only CPC seed with TCC, then proves that seed can compile and run a C program."); return 0;
        } else return 2;
    }
    if(!authorized) { fprintf(stderr,"TCC seed compilation requires -RunExternal.\n"); return 2; }
    if(!nt_exists(tcc)) nt_die("TCC seed host missing",tcc);
    nt_mkdirs(out); nt_join(seed,sizeof seed,out,"cpc.exe"); DeleteFileA(seed);
    compiler_arguments(compile,tcc,root,NULL,seed);
    if(!run_checked("TCC C seed build",compile,root,600000,&seed_seconds)||!nt_exists(seed)) return 1;
    nt_join(test_source,sizeof test_source,root,"Compatibility\\tests\\c_compat\\pass\\test_basic_arithmetic.c");
    nt_join(test_exe,sizeof test_exe,out,"c-seed-check.exe"); snprintf(bflag,sizeof bflag,"-B%s",root);
    test[0]=seed; test[1]=bflag; test[2]=test_source; test[3]="-o"; test[4]=test_exe; test[5]=NULL;
    if(!run_checked("seed C compilation",test,root,120000,&test_seconds)) return 1;
    run[0]=test_exe; run[1]=NULL;
    if(!run_checked("seed C output",run,root,30000,NULL)) return 1;
    run[0]=seed; run[1]="--version"; run[2]=NULL;
    if(!run_checked("seed version smoke",run,root,30000,NULL)) return 1;
    nt_join(self,sizeof self,out,"cpc-self.exe");
    compiler_arguments(compile,seed,root,root,self);
    if(!run_checked("seed self-compilation as C",compile,root,600000,&self_seconds)) return 1;
    run[0]=self; run[1]="--version"; run[2]=NULL;
    if(!run_checked("self-compiled seed smoke",run,root,30000,NULL)) return 1;
    printf("PASS TCC -> C-only CPC seed -> C-only self compile -> C program (TCC %.3fs, CPC %.3fs, check %.3fs)\n",
           seed_seconds,self_seconds,test_seconds);
    return 0;
}
