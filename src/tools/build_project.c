/* Native serial CPC project driver. Build with scripts/windows/build-project-tools.cmd. */
#include "build_project_data.inc"
typedef unsigned long long U64;
typedef struct List {
    char **v;
    int n, cap;
}
List;
typedef struct Stamp {
    U64 time, size;
    unsigned kind;
}
Stamp;
typedef struct Entry {
    char *path;
    Stamp stamp;
    U64 names;
    int scanned;
    struct Entry *next;
}
Entry;
static Entry *files[8192];
static char *compiler, *outdir, *exepath, *manifestpath, *self, *repo, *context;
static int rebuild, unity, skiplink, allowwarnings, timeout_seconds=60, schema;
static int unity_batch_size=32;
static double compiler_seconds, resource_seconds;
static J *measurements;
static const char *environment[] = {
    "INCLUDE","LIB","LIBRARY_PATH","CPATH","C_INCLUDE_PATH","CPLUS_INCLUDE_PATH",
    "VCToolsInstallDir","VCToolsVersion","WindowsSdkDir","WindowsSDKVersion","PATH"
};
static void add(List *l,const char *s) {
    if(l->n==l->cap){
        l->cap=l->cap?l->cap*2:16;
        l->v=realloc(l->v,l->cap*sizeof(char*));
        if(!l->v)die("Out of memory");
    }
    l->v[l->n++]=str(s);
}
static void distinct(List *l,const char *s) {
    int i;
    for(i=0;i<l->n;i++)if(!_stricmp(l->v[i],s))return;
    add(l,s);
}
static void extend(List *a,List *b) {
    int i;
    for(i=0;i<b->n;i++)add(a,b->v[i]);
}
static void json_list(List *l,J *a,const char *prefix) {
    J *v;
    for(v=a?a->child:NULL;v;v=v->next)add(l,cat(prefix,v->s));
}
static double seconds(void) {
    LARGE_INTEGER t,f;
    QueryPerformanceCounter(&t);
    QueryPerformanceFrequency(&f);
    return (double)t.QuadPart/f.QuadPart;
}
static U64 hash_bytes(U64 h,const char *s) {
    for(;*s;s++)h=(h^(unsigned char)*s)*1099511628211ULL;
    return h;
}
static Stamp stat_path(const char *p) {
    Stamp s={
        0
    };
    WIN32_FILE_ATTRIBUTE_DATA d;
    wchar_t *w=wide(p);
    if(GetFileAttributesExW(w,GetFileExInfoStandard,&d)){
        s.kind=(d.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)?1:0;
        s.time=((U64)d.ftLastWriteTime.dwHighDateTime<<32)|d.ftLastWriteTime.dwLowDateTime;
        s.size=((U64)d.nFileSizeHigh<<32)|d.nFileSizeLow;
    }
    else {
        DWORD e=GetLastError();
        if(e!=ERROR_FILE_NOT_FOUND&&e!=ERROR_PATH_NOT_FOUND)fail(w);
        s.kind=2;
    }
    free(w);
    return s;
}
static unsigned path_bucket(const char *path) {
    U64 hash=1469598103934665603ULL;
    for(;*path;path++)hash=(hash^(unsigned char)tolower((unsigned char)*path))*1099511628211ULL;
    return (unsigned)(hash%8192);
}
static Entry *entry(const char *path) {
    char *p;
    unsigned b=path_bucket(path);
    Entry *e;
    /* Saved dependency paths are already canonical. Check the shared table
       before allocating strings or calling Windows path APIs again. */
    for(e=files[b];e;e=e->next)if(!_stricmp(e->path,path))return e;
    p=absolute(".",path);
    b=path_bucket(p);
    for(e=files[b];e;e=e->next)if(!_stricmp(e->path,p)){
        free(p);
        return e;
    }
    e=alloc(sizeof(*e));
    e->path=p;
    e->stamp=stat_path(p);
    e->next=files[b];
    files[b]=e;
    return e;
}
static void refresh(const char *p) {
    Entry *e=entry(p);
    e->stamp=stat_path(e->path);
}
static int stamp_equal(Stamp a,Stamp b) {
    return a.kind==b.kind&&a.time==b.time&&a.size==b.size;
}
static int compare_names(const void *a,const void *b) {
    return strcmp(*(char*const*)a,*(char*const*)b);
}
static int header_name(const char *s) {
    const char *p=strrchr(s,'.');
    return !p||!_stricmp(p,".h")||!_stricmp(p,".hpp")||!_stricmp(p,".hxx")||!_stricmp(p,".inc")||!_stricmp(p,".inl");
}
static U64 directory_names(Entry *e) {
    List names={
        0
    };
    WIN32_FIND_DATAW d;
    HANDLE h;
    wchar_t *w;
    int i;
    U64 result=1469598103934665603ULL;
    if(e->scanned)return e->names;
    w=wide(cat(e->path,"/*"));
    h=FindFirstFileW(w,&d);
    free(w);
    if(h!=INVALID_HANDLE_VALUE){
        do {
            char *s=utf8(d.cFileName);
            if(!(d.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)&&header_name(s))add(&names,s);
            free(s);
        }
        while(FindNextFileW(h,&d));
        if(GetLastError()!=ERROR_NO_MORE_FILES)die("Cannot scan include directory");
        FindClose(h);
    }
    else if(GetLastError()!=ERROR_FILE_NOT_FOUND&&GetLastError()!=ERROR_PATH_NOT_FOUND)die("Cannot scan include directory");
    qsort(names.v,names.n,sizeof(char*),compare_names);
    for(i=0;i<names.n;i++){
        result=hash_bytes(hash_bytes(result,names.v[i]),"\n");
        free(names.v[i]);
    }
    free(names.v);
    e->scanned=1;
    e->names=result;
    return result;
}
static void u32(FILE *f,unsigned v) {
    if(fwrite(&v,4,1,f)!=1)die("Cache write failed");
}
static void u64(FILE *f,U64 v) {
    if(fwrite(&v,8,1,f)!=1)die("Cache write failed");
}
static void string_write(FILE *f,const char *s) {
    unsigned n=(unsigned)strlen(s);
    u32(f,n);
    if(fwrite(s,1,n,f)!=n)die("Cache write failed");
}
static unsigned read32(FILE *f,int *ok) {
    unsigned n=0;
    if(fread(&n,4,1,f)!=1)*ok=0;
    return n;
}
static U64 read64(FILE *f,int *ok) {
    U64 n=0;
    if(fread(&n,8,1,f)!=1)*ok=0;
    return n;
}
static char *string_read(FILE *f,int *ok) {
    unsigned n=read32(f,ok);
    char *s;
    if(!*ok||n>16*1024*1024){
        *ok=0;
        return str("");
    }
    s=alloc(n+1);
    if(fread(s,1,n,f)!=n||memchr(s,0,n))*ok=0;
    return s;
}
static void stamp_write(FILE *f,Stamp s) {
    u32(f,s.kind);
    u64(f,s.time);
    u64(f,s.size);
}
static Stamp stamp_read(FILE *f,int *ok) {
    Stamp s;
    s.kind=read32(f,ok);
    s.time=read64(f,ok);
    s.size=read64(f,ok);
    if(s.kind>2)*ok=0;
    return s;
}
static char *state_path(const char *p) {
    return cat(p,".state.bin");
}
static int cache_valid(const char *output,const char *key) {
    FILE *f;
    char magic[8],*saved;
    int ok=1;
    unsigned i,n;
    Stamp s;
    if(rebuild)return 0;
    f=file_open(state_path(output),L"rb");
    if(!f)return 0;
    if(fread(magic,1,8,f)!=8||memcmp(magic,"CPCSTA01",8)){
        fclose(f);
        return 0;
    }
    saved=string_read(f,&ok);
    if(strcmp(saved,key))ok=0;
    free(saved);
    s=stamp_read(f,&ok);
    if(s.kind!=0||!stamp_equal(s,entry(output)->stamp))ok=0;
    n=read32(f,&ok);
    if(n>1000000)ok=0;
    for(i=0;ok&&i<n;i++){
        char *p=string_read(f,&ok);
        s=stamp_read(f,&ok);
        if(ok&&(s.kind!=0||!stamp_equal(s,entry(p)->stamp)))ok=0;
        free(p);
    }
    n=read32(f,&ok);
    if(n>1000000)ok=0;
    for(i=0;ok&&i<n;i++){
        char *p=string_read(f,&ok);
        U64 time=read64(f,&ok),names=read64(f,&ok);
        if(ok){
            Entry *e=entry(p);
            if(!e->scanned&&e->stamp.time==time){
                e->scanned=1;
                e->names=names;
            }
            if(directory_names(e)!=names)ok=0;
        }
        free(p);
    }
    if(ok&&fgetc(f)!=EOF)ok=0;
    fclose(f);
    if(ok)entry(state_path(output));
    return ok;
}
static void cache_save(const char *output,const char *key,List *inputs,List *dirs) {
    FILE *f;
    List unique_inputs={
        0
    },unique_dirs={
        0
    };
    int i;
    for(i=0;i<inputs->n;i++)if(_stricmp(inputs->v[i],output))distinct(&unique_inputs,absolute(".",inputs->v[i]));
    for(i=0;i<dirs->n;i++)distinct(&unique_dirs,absolute(".",dirs->v[i]));
    refresh(output);
    f=file_open(state_path(output),L"wb");
    if(!f)die("Cannot write cache");
    fwrite("CPCSTA01",1,8,f);
    string_write(f,key);
    stamp_write(f,entry(output)->stamp);
    u32(f,unique_inputs.n);
    for(i=0;i<unique_inputs.n;i++){
        Entry *e=entry(unique_inputs.v[i]);
        string_write(f,e->path);
        stamp_write(f,e->stamp);
    }
    u32(f,unique_dirs.n);
    for(i=0;i<unique_dirs.n;i++){
        Entry *e=entry(unique_dirs.v[i]);
        string_write(f,e->path);
        u64(f,e->stamp.time);
        u64(f,directory_names(e));
    }
    if(fclose(f))die("Cannot finish cache");
    refresh(state_path(output));
}
static void arg_quote(Buf *b,const char *s,int response) {
    if(b->n)bs(b," ");
    bs(b,"\"");
    while(*s){
        unsigned n=0,i;
        if(response){
            if(*s=='\n'||*s=='\r')die("Newline in compiler argument");
            if(*s=='\\'||*s=='\"')bs(b,"\\");
            bn(b,s++,1);
            continue;
        }
        while(*s=='\\'){
            n++;
            s++;
        }
        for(i=0;i<n*((!*s||*s=='\"')?2:1);i++)bs(b,"\\");
        if(!*s)break;
        if(*s=='\"')bs(b,"\\");
        bn(b,s++,1);
    }
    bs(b,"\"");
}
static char *command_line(const char *tool,List *args,int response) {
    Buf b={
        0
    };
    int i;
    if(tool)arg_quote(&b,tool,response);
    for(i=0;i<args->n;i++)arg_quote(&b,args->v[i],response);
    return b.s?b.s:str("");
}
static char *build_key(const char *tool,List *args,const char *cwd) {
    Buf b={
        0
    };
    char stamp[100];
    Stamp s=entry(tool)->stamp;
    sprintf(stamp,"%llu:%llu",s.time,s.size);
    bs(&b,context);
    bs(&b,"\n");
    bs(&b,tool);
    bs(&b,stamp);
    bs(&b,"\n");
    bs(&b,cwd);
    bs(&b,"\n");
    bs(&b,command_line(NULL,args,1));
    return b.s;
}
static void dependencies(List *l,const char *dep,const char *cwd) {
    char *text=read_utf8(dep),*p=text,*start;
    Buf token={
        0
    };
    for(;*p;p++)if(*p==':'&&isspace((unsigned char)p[1])){
        p++;
        break;
    }
    if(!*p)die("Invalid dependency file");
    while(*p){
        while(isspace((unsigned char)*p))p++;
        if(!*p)break;
        token.n=0;
        if(token.s)token.s[0]=0;
        start=p;
        while(*p&&!isspace((unsigned char)*p)){
            if(*p=='\\'&&(p[1]=='\r'||p[1]=='\n')){
                p++;
                if(*p=='\r')p++;
                if(*p=='\n')p++;
                break;
            }
            if(*p=='\\'&&(isspace((unsigned char)p[1])||p[1]=='#'))p++;
            if(*p=='$'&&p[1]=='$')p++;
            bn(&token,p++,1);
        }
        if(token.n)distinct(l,absolute(cwd,token.s));
        if(p==start)p++;
    }
    free(token.s);
    free(text);
}
typedef struct Project Project;
typedef struct Job {
    char *source,*object,*dep,*label,*key,*group;
    List flags,inputs;
    Project *project;
    int dirty;
}
Job;
struct Project {
    J *data;
    char *directory,*label,*archive,*rc,*cvtres;
    List objects,resources,sdkincludes,libraries;
    int selected,visiting,useobjects;
};
static Project *projects;
static int project_count;
static Project *app;
static Job *jobs;
static int job_count,source_count,dirty_count;
static List linked_indices;
static void number(J *j,const char *key,double n) {
    char s[80];
    sprintf(s,"%.6f",n);
    put(j,key,jnew(JN,s));
}
static int batch_done,batch_started,batch_count;
static Job **batch_jobs;
static double batch_deadline;
static Buf diagnostics;
static void cache_job(Job *j) {
    List inputs={
        0
    },dirs={
        0
    };
    int k;
    dependencies(&inputs,j->dep,j->project->directory);
    extend(&inputs,&j->inputs);
    for(k=0;k<j->flags.n;k++)if(!strncmp(j->flags.v[k],"-I",2))distinct(&dirs,absolute(j->project->directory,j->flags.v[k]+2));
    for(k=0;k<inputs.n;k++)distinct(&dirs,directory(inputs.v[k]));
    cache_save(j->object,j->key,&inputs,&dirs);
}
static double build_start;
static const char *file_name(const char *p) {
    const char *s=strrchr(p,'\\'),*t=strrchr(p,'/');
    if(t&&(!s||t>s))s=t;
    return s?s+1:p;
}
static unsigned long long elapsed_ms(double begin) {
    return (unsigned long long)((seconds()-begin)*1000.0+0.5);
}
static void complete(void) {
    printf("Complete (%llums elapsed)\n",elapsed_ms(build_start));
}
static void diagnostic_line(const char *line) {
    unsigned idx,ms;
    int code;
    if(sscanf(line,"# cprime batch start %u",&idx)==1){
        if(batch_started||idx!=(unsigned)batch_done+1||batch_done>=batch_count)die("Invalid compiler batch start");
        batch_started=1;
        batch_deadline=seconds();
    }
    else if(sscanf(line,"# cprime batch end %u %d %u",&idx,&code,&ms)==3){
        Job *j;
        int warnings=0;
        char *p;
        if(!batch_started||idx!=(unsigned)batch_done+1||batch_done>=batch_count)die("Invalid compiler batch result");
        j=batch_jobs[batch_done];
        for(p=diagnostics.s;p&&(p=strstr(p,"warning:"))!=NULL;p+=8)warnings++;
        write_utf8(cat(outdir,cat("/",cat(j->label,".err.log"))),diagnostics.s?diagnostics.s:"");
        printf("%ums %s",ms,file_name(j->inputs.v[0]));
        if(j->inputs.n>1)printf(" (+%d more)",j->inputs.n-1);
        if(warnings)printf(" warnings=%d",warnings);
        printf("\n");
        fflush(stdout);
        if(code||!exists(j->object)){
            if(diagnostics.s)fputs(diagnostics.s,stderr);
            die("Compiler did not produce a successful object");
        }
        /* Keep completed units cached even if a later unit is cancelled. */
        cache_job(j);
        diagnostics.n=0;
        if(diagnostics.s)diagnostics.s[0]=0;
        batch_done++;
        batch_started=0;
        batch_deadline=seconds();
    }
    else {
        bs(&diagnostics,line);
        bs(&diagnostics,"\n");
    }
}
static void run_tool(const char *tool,List *args,const char *cwd,const char *label,int batch,int resource) {
    STARTUPINFOW si={
        0
    };
    PROCESS_INFORMATION pi={
        0
    };
    SECURITY_ATTRIBUTES sa={
        sizeof(sa),NULL,TRUE
    };
    HANDLE readpipe,writepipe,output;
    HANDLE process_job;
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits={
        0
    };
    wchar_t *cmd=wide(command_line(tool,args,0)),*exe=wide(tool),*wd=wide(cwd),*log=wide(cat(outdir,cat("/",cat(label,".log"))));
    Buf err={
        0
    },line={
        0
    };
    DWORD code=0;
    int ended=0,timedout=0;
    double begin=seconds(),elapsed,cpu_seconds=0;
    J *m;
    if(wcslen(cmd)>=CAP)die("Tool command exceeds Windows command-line limit");
    if(!CreatePipe(&readpipe,&writepipe,&sa,0)||!SetHandleInformation(readpipe,HANDLE_FLAG_INHERIT,0))die("Cannot create tool pipe");
    output=CreateFileW(log,GENERIC_WRITE,FILE_SHARE_READ,&sa,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    if(output==INVALID_HANDLE_VALUE)fail(log);
    si.cb=sizeof(si);
    si.dwFlags=STARTF_USESTDHANDLES;
    si.hStdOutput=output;
    si.hStdError=writepipe;
    si.hStdInput=GetStdHandle(STD_INPUT_HANDLE);
    process_job=CreateJobObjectW(NULL,NULL);
    limits.BasicLimitInformation.LimitFlags=JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if(!process_job||!SetInformationJobObject(process_job,JobObjectExtendedLimitInformation,&limits,sizeof(limits)))die("Cannot create compiler lifetime guard");
    if(!CreateProcessW(exe,cmd,NULL,NULL,TRUE,CREATE_NO_WINDOW|CREATE_SUSPENDED,NULL,wd,&si,&pi))fail(exe);
    if(!AssignProcessToJobObject(process_job,pi.hProcess)){
        TerminateProcess(pi.hProcess,1);
        die("Cannot guard compiler lifetime");
    }
    ResumeThread(pi.hThread);
    CloseHandle(writepipe);
    CloseHandle(output);
    batch_done=0;
    batch_started=0;
    batch_deadline=seconds();
    for(;;){
        DWORD available=0,got;
        char buffer[8192];
        if(!ended&&seconds()-(batch?batch_deadline:begin)>=timeout_seconds){
            TerminateProcess(pi.hProcess,1);
            WaitForSingleObject(pi.hProcess,INFINITE);
            timedout=1;
            ended=1;
        }
        if(PeekNamedPipe(readpipe,NULL,0,NULL,&available,NULL)&&available){
            if(!ReadFile(readpipe,buffer,available>sizeof(buffer)?sizeof(buffer):available,&got,NULL))break;
            bn(&err,buffer,got);
            if(batch){
                DWORD i;
                for(i=0;i<got;i++){
                    if(buffer[i]=='\n'){
                        diagnostic_line(line.s?line.s:"");
                        line.n=0;
                        if(line.s)line.s[0]=0;
                    }
                    else if(buffer[i]!='\r')bn(&line,buffer+i,1);
                }
            }
            continue;
        }
        if(ended)break;
        if(WaitForSingleObject(pi.hProcess,1)==WAIT_OBJECT_0){
            ended=1;
            continue;
        }
        if(seconds()-(batch?batch_deadline:begin)>=timeout_seconds){
            TerminateProcess(pi.hProcess,1);
            WaitForSingleObject(pi.hProcess,INFINITE);
            timedout=1;
            ended=1;
        }
    }
    GetExitCodeProcess(pi.hProcess,&code);
    elapsed=seconds()-begin;
    {
        FILETIME created,exited,kernel,user;
        if(GetProcessTimes(pi.hProcess,&created,&exited,&kernel,&user))
            cpu_seconds=(((U64)kernel.dwHighDateTime<<32)+kernel.dwLowDateTime
                        +((U64)user.dwHighDateTime<<32)+user.dwLowDateTime)/10000000.0;
    }
    CloseHandle(readpipe);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    CloseHandle(process_job);
    if(batch&&line.n)diagnostic_line(line.s);
    write_utf8(cat(outdir,cat("/",cat(label,".err.log"))),err.s?err.s:"");
    m=jnew(JO,"");
    set(m,"Label",label);
    set(m,"Kind",resource==2?"driver":resource?"resource":"compiler");
    number(m,"Seconds",elapsed);
    number(m,"CpuSeconds",cpu_seconds);
    ja(measurements,m);
    if(resource==1)resource_seconds+=elapsed;
    else if(!resource)compiler_seconds+=elapsed;
    if(timedout||code||(batch&&(batch_done!=batch_count||batch_started))){
        char *stdout_text=read_utf8(cat(outdir,cat("/",cat(label,".log"))));
        fputs(stdout_text,stderr);
        if(err.s)fputs(err.s,stderr);
        if(timedout)fprintf(stderr,"Compile timed out after %ds\n",timeout_seconds);
        die(label);
    }
    free(cmd);
    free(exe);
    free(wd);
    free(log);
    free(err.s);
    free(line.s);
}
static int cached_tool(const char *tool,List *args,const char *cwd,const char *label,const char *output,List *inputs,List *dirs,const char *dep,int resource) {
    char *key=build_key(tool,args,cwd);
    if(cache_valid(output,key))return 0;
    remove_file(state_path(output));
    remove_file(output);
    refresh(output);
    run_tool(tool,args,cwd,label,0,resource);
    if(!exists(output))die("Tool did not produce output");
    if(dep)dependencies(inputs,dep,cwd);
    cache_save(output,key,inputs,dirs);
    return 1;
}
static const char *env_value(const char *name) {
    const char *s=getenv(name);
    return s?s:"";
}
static const char *setting(J *project,J *source,const char *name) {
    J *v=get(source,name);
    return v?v->s:val(project,name);
}
static void split_flags(List *flags,const char *text,const char *prefix,const char *cwd,int forced) {
    char *copy=str(text),*s=copy,*p;
    do{
        p=strchr(s,';');
        if(p)*p=0;
        if(*s){
            if(forced){
                add(flags,"-include");
                add(flags,absolute(cwd,s));
            }
            else add(flags,cat(prefix,s));
        }
        s=p?p+1:NULL;
    }
    while(s);
    free(copy);
}
static List compile_flags(Project *p,J *source) {
    List flags={
        0
    };
    J *a=get(p->data,"compileSettings"),*b=get(source,"settings"),*includes,*defines;
    const char *optimization=setting(a,b,"Optimization");
    if(!strcmp(optimization,"Disabled"))add(&flags,"-O0");
    else if(!strcmp(optimization,"MinSpace"))add(&flags,"-Os");
    else if(!strcmp(optimization,"MaxSpeed")||!strcmp(optimization,"Full"))add(&flags,"-O2");
    if(!allowwarnings&&!strcmp(setting(a,b,"TreatWarningAsError"),"true"))add(&flags,"-Werror");
    split_flags(&flags,setting(a,b,"UndefinePreprocessorDefinitions"),"-U",p->directory,0);
    split_flags(&flags,setting(a,b,"ForcedIncludeFiles"),"",p->directory,1);
    includes=get(source,"includeDirectories");
    defines=get(source,"defines");
    if(schema==1||!includes)json_list(&flags,get(p->data,"includeDirectories"),"-I");
    if(includes)json_list(&flags,includes,"-I");
    if(schema==1||!defines)json_list(&flags,get(p->data,"defines"),"-D");
    if(defines)json_list(&flags,defines,"-D");
    return flags;
}
static Project *project_named(const char *name) {
    int i;
    for(i=0;i<project_count;i++)if(!_stricmp(val(projects[i].data,"projectFile"),name))return &projects[i];
    die(cat("Referenced project is missing: ",name));
    return NULL;
}
static void select_project(Project *p) {
    J *r,*refs=get(p->data,"projectReferences");
    if(p->visiting)die("Project reference cycle");
    if(p->selected)return;
    p->visiting=1;
    p->selected=1;
    for(r=refs?refs->child:NULL;r;r=r->next){
        Project *d=project_named(val(r,"projectFile"));
        if(strcmp(val(r,"linkLibraryDependencies"),"true"))continue;
        if(!strcmp(val(r,"useLibraryDependencyInputs"),"true"))d->useobjects=1;
        select_project(d);
    }
    p->visiting=0;
}
static void order_projects(void) {
    int *done=alloc(project_count*sizeof(int)),count=0,i,j;
    while(count<project_count){
        int progress=0;
        for(i=0;i<project_count;i++){
            int blocked=0;
            char idx[32];
            if(done[i])continue;
            if(!projects[i].selected){
                done[i]=1;
                count++;
                continue;
            }
            for(j=0;j<project_count&&!blocked;j++)if(!done[j]&&projects[j].selected){
                J *refs=get(projects[j].data,"projectReferences"),*r;
                for(r=refs?refs->child:NULL;r;r=r->next)if(!strcmp(val(r,"linkLibraryDependencies"),"true")&&!_stricmp(val(r,"projectFile"),val(projects[i].data,"projectFile")))blocked=1;
            }
            if(blocked)continue;
            sprintf(idx,"%d",i);
            add(&linked_indices,idx);
            done[i]=1;
            count++;
            progress=1;
        }
        if(!progress&&count<project_count)die("Cannot order project graph");
    }
    free(done);
}
static void make_jobs(void) {
    int i;
    for(i=0;i<project_count;i++){
        Project *p=&projects[i];
        J *sources=get(p->data,"sources"),*s;
        for(s=sources?sources->child:NULL;s;s=s->next){
            List flags=compile_flags(p,s);
            char label[64],*source=absolute(p->directory,val(s,"path")),*group=cat(command_line(NULL,&flags,1),cat("\n",val(s,"unityGroup")));
            Job *job=NULL;
            int k;
            const char *ext=strrchr(source,'.');
            if(!exists(source))die(cat("Selected source is missing: ",source));
            if(unity&&ext&&(!_stricmp(ext,".cpp")||!_stricmp(ext,".cxx")||!_stricmp(ext,".cc")))for(k=0;k<job_count;k++)if(jobs[k].project==p&&jobs[k].group&&!strcmp(jobs[k].group,group)&&jobs[k].inputs.n<unity_batch_size){
                job=&jobs[k];
                break;
            }
            if(!job){
                jobs=realloc(jobs,(job_count+1)*sizeof(Job));
                if(!jobs)die("Out of memory");
                job=&jobs[job_count++];
                memset(job,0,sizeof(*job));
                sprintf(label,"source_%04d",source_count);
                job->label=str(label);
                job->project=p;
                job->flags=flags;
                job->source=source;
                if(unity&&ext&&(!_stricmp(ext,".cpp")||!_stricmp(ext,".cxx")||!_stricmp(ext,".cc")))job->group=group;
            }
            add(&job->inputs,source);
            source_count++;
        }
    }
    for(i=0;i<job_count;i++){
        Job *j=&jobs[i];
        char name[64],*identity,*q;
        Buf bkey={0};
        U64 hash;
        int k;
        /* Cache identity belongs to the source and its project/settings, never
           its position in the exported source list. Include every unity member
           so changing a group cannot reuse an object for different inputs. */
        bs(&bkey,j->project->directory);
        bs(&bkey,"\n");
        bs(&bkey,val(j->project->data,"projectFile"));
        bs(&bkey,"\n");
        bs(&bkey,val(j->project->data,"targetName"));
        for(k=0;k<j->inputs.n;k++){
            bs(&bkey,"\n");
            bs(&bkey,j->inputs.v[k]);
        }
        identity=bkey.s;
        for(q=identity;*q;q++){
            if(*q=='\\')*q='/';
            *q=(char)tolower((unsigned char)*q);
        }
        hash=hash_bytes(1469598103934665603ULL,identity);
        free(identity);
        identity=command_line(NULL,&j->flags,1);
        hash=hash_bytes(hash_bytes(hash,"\n"),identity);
        free(identity);
        sprintf(name,"source_%016llx",hash);
        j->object=absolute(outdir,cat(name,".obj"));
        j->dep=absolute(outdir,cat(name,".d"));
        add(&j->project->objects,j->object);
        if(j->inputs.n>1){
            Buf b={
                0
            };
            int k;
            j->source=absolute(outdir,cat(name,".cpp"));
            for(k=0;k<j->inputs.n;k++){
                char *p=str(j->inputs.v[k]),*q;
                for(q=p;*q;q++)if(*q=='\\')*q='/';
                bs(&b,"#include \"");
                bs(&b,p);
                bs(&b,"\"\n");
                free(p);
            }
            write_utf8(j->source,b.s);
            free(b.s);
        }
    }
}
static List job_arguments(Job *j) {
    List a={
        0
    };
    extend(&a,&j->flags);
    add(&a,"-MD");
    add(&a,"-MF");
    add(&a,j->dep);
    add(&a,"-c");
    add(&a,j->source);
    add(&a,"-o");
    add(&a,j->object);
    return a;
}
static void compile_jobs(void) {
    int i,k,batch_index=0;
#ifdef BUILD_PROJECT_EXTERNAL
    for(i=0;i<job_count;i++)if(jobs[i].dirty){
        List args=job_arguments(&jobs[i]);
        char label[64];
        sprintf(label,"compile_%04d",batch_index++);
        batch_jobs=alloc(sizeof(Job*));
        batch_jobs[0]=&jobs[i];
        batch_count=1;
        run_tool(compiler,&args,jobs[i].project->directory,label,1,0);
        free(batch_jobs);
    }
#else
    for(i=0;i<job_count;i++)if(jobs[i].dirty){
        Buf response={
            0
        };
        List args={
            0
        };
        char label[64],*path;
        batch_jobs=alloc(job_count*sizeof(Job*));
        batch_count=0;
        for(k=i;k<job_count;k++){
            List a;
            if(!jobs[k].dirty)continue;
            if(strcmp(jobs[k].project->directory,jobs[i].project->directory))break;
            batch_jobs[batch_count++]=&jobs[k];
            a=job_arguments(&jobs[k]);
            bs(&response,command_line(NULL,&a,1));
            bs(&response,"\n");
        }
        sprintf(label,"compile_batch_%04d",batch_index++);
        path=absolute(outdir,cat(label,".txt"));
        write_utf8(path,response.s);
        add(&args,cat("@",path));
        run_tool(compiler,&args,jobs[i].project->directory,label,1,0);
        free(batch_jobs);
        i=k-1;
    }
#endif
}
static int newer_version(const char *a,const char *b) {
    unsigned av[4]={
        0
    },bv[4]={
        0
    };
    int i;
    sscanf(a,"%u.%u.%u.%u",av,av+1,av+2,av+3);
    sscanf(b,"%u.%u.%u.%u",bv,bv+1,bv+2,bv+3);
    for(i=0;i<4;i++)if(av[i]!=bv[i])return av[i]>bv[i];
    return 0;
}
static char *latest_directory(const char *root,const char *requested) {
    WIN32_FIND_DATAW d;
    HANDLE h;
    char *chosen=str("");
    wchar_t *w=wide(cat(root,"/*"));
    h=FindFirstFileW(w,&d);
    free(w);
    if(h!=INVALID_HANDLE_VALUE){
        do{
            char *s=utf8(d.cFileName);
            if((d.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)&&isdigit((unsigned char)*s)&&(!*requested||!strcmp(requested,"10.0")||!strcmp(requested,s))&&newer_version(s,chosen)){
                free(chosen);
                chosen=str(s);
            }
            free(s);
        }
        while(FindNextFileW(h,&d));
        FindClose(h);
    }
    return chosen;
}
static char *trim_end(char *s) {
    size_t n=strlen(s);
    while(n&&(isspace((unsigned char)s[n-1])||s[n-1]=='/'||s[n-1]=='\\'))s[--n]=0;
    return s;
}
static char *vs_install;
static void select_tools(Project *p) {
    char *kit=str(env_value("WindowsSdkDir")),*version,*vc=str(env_value("VCToolsInstallDir"));
    const char *request=val(p->data,"windowsSdkVersion");
    J *resources=get(p->data,"resources");
    if(!*kit){
        HKEY key;
        wchar_t value[CAP];
        DWORD size=sizeof(value);
        if(RegOpenKeyExW(HKEY_LOCAL_MACHINE,L"SOFTWARE\\Microsoft\\Windows Kits\\Installed Roots",0,KEY_READ,&key)==ERROR_SUCCESS){
            if(RegQueryValueExW(key,L"KitsRoot10",NULL,NULL,(BYTE*)value,&size)==ERROR_SUCCESS)kit=utf8(value);
            RegCloseKey(key);
        }
    }
    if(!*kit)kit=cat(env_value("ProgramFiles(x86)"),"/Windows Kits/10");
    if(!*request)request=env_value("WindowsSDKVersion");
    version=latest_directory(cat(kit,"/Lib"),trim_end(str(request)));
    if(*version){
        char *lib=cat(cat(kit,"/Lib/"),version),*inc=cat(cat(kit,"/Include/"),version);
        p->rc=cat(cat(cat(kit,"/bin/"),version),"/x64/rc.exe");
        add(&p->libraries,cat(lib,"/um/x64"));
        add(&p->libraries,cat(lib,"/ucrt/x64"));
        add(&p->sdkincludes,cat(inc,"/um"));
        add(&p->sdkincludes,cat(inc,"/shared"));
    }
    else if(*request||(resources&&resources->child))die("Selected Windows SDK was not found");
    request=val(p->data,"msvcToolsVersion");
    if(!*request)request=env_value("VCToolsVersion");
    if(!*vc){
        char *finder=cat(env_value("ProgramFiles(x86)"),"/Microsoft Visual Studio/Installer/vswhere.exe");
        if(!vs_install&&exists(finder)){
            List a={
                0
            };
            char *log=absolute(outdir,"find_tools.log");
            add(&a,"-latest");
            add(&a,"-products");
            add(&a,"*");
            add(&a,"-requires");
            add(&a,"Microsoft.VisualStudio.Component.VC.Tools.x86.x64");
            add(&a,"-property");
            add(&a,"installationPath");
            run_tool(finder,&a,repo,"find_tools",0,2);
            vs_install=trim_end(read_utf8(log));
        }
        if(vs_install&&*vs_install){
            char *base=cat(vs_install,"/VC/Tools/MSVC");
            version=latest_directory(base,trim_end(str(request)));
            if(*version)vc=cat(cat(base,"/"),version);
        }
    }
    else if(*request){
        vc=absolute(directory(trim_end(vc)),trim_end(str(request)));
    }
    if(*vc&&exists(cat(vc,"/lib/x64"))){
        add(&p->libraries,cat(vc,"/lib/x64"));
        p->cvtres=cat(vc,"/bin/Hostx64/x64/cvtres.exe");
    }
    else if(*request)die("Selected Visual C++ x64 libraries were not found");
}
static void resource_tree(List *inputs,const char *root,List *visited) {
    WIN32_FIND_DATAW d;
    HANDLE h;
    wchar_t *w;
    int before=visited->n;
    distinct(visited,root);
    if(before==visited->n)return;
    w=wide(cat(root,"/*"));
    h=FindFirstFileW(w,&d);
    free(w);
    if(h==INVALID_HANDLE_VALUE)return;
    do {
        char *name=utf8(d.cFileName),*path=absolute(root,name);
        const char *ext=strrchr(name,'.');
        if(d.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY){
            if(strcmp(name,".")&&strcmp(name,"..")&&!(d.dwFileAttributes&FILE_ATTRIBUTE_REPARSE_POINT))resource_tree(inputs,path,visited);
        }
        else if(ext&&(!_stricmp(ext,".h")||!_stricmp(ext,".hpp")||!_stricmp(ext,".inc")||!_stricmp(ext,".inl")||!_stricmp(ext,".rc")||!_stricmp(ext,".rc2")))distinct(inputs,path);
        free(name);
        free(path);
    }
    while(FindNextFileW(h,&d));
    FindClose(h);
}
static int macro_include(const char *text) {
    const char *p=text;
    while(*p){
        while(*p==' '||*p=='\t')p++;
        if(*p=='#'){
            p++;
            while(*p==' '||*p=='\t')p++;
            if(!strncmp(p,"include",7)&&isspace((unsigned char)p[7])){
                p+=7;
                while(*p==' '||*p=='\t')p++;
                if(*p!='\"'&&*p!='<')return 1;
            }
        }
        while(*p&&*p!='\n')p++;
        if(*p)p++;
    }
    return 0;
}
static void resource_references(List *inputs,const char *source,List *dirs,const char *extra) {
    int i,expanded=0;
    List visited={
        0
    };
    distinct(inputs,source);
    for(i=0;i<inputs->n;i++){
        char *text=read_utf8(inputs->v[i]),*p=text;
        const char *ext=strrchr(inputs->v[i],'.');
        if(ext&&_stricmp(ext,".rc")&&_stricmp(ext,".rc2")&&_stricmp(ext,".h")&&_stricmp(ext,".hpp")&&_stricmp(ext,".inc")){
            free(text);
            continue;
        }
        if(!i&&extra)p=cat(text,extra);
        if(!expanded&&macro_include(p)){
            int k;
            expanded=1;
            for(k=0;k<dirs->n;k++)resource_tree(inputs,dirs->v[k],&visited);
        }
        while(*p){
            char end,*s,*name;
            int k;
            if(*p!='\"'&&*p!='<'){
                p++;
                continue;
            }
            end=*p++=='<'?'>':'\"';
            s=p;
            while(*p&&*p!=end&&*p!='\n')p++;
            if(*p!=end)continue;
            name=alloc(p-s+1);
            memcpy(name,s,p-s);
            p++;
            for(k=-1;k<dirs->n;k++){
                char *file=absolute(k<0?directory(inputs->v[i]):dirs->v[k],name);
                if(exists(file)&&entry(file)->stamp.kind==0){
                    distinct(inputs,file);
                    break;
                }
            }
            free(name);
        }
        free(text);
    }
}
static void resources_and_archives(void) {
    int i,k;
    for(i=0;i<project_count;i++){
        Project *p=&projects[i];
        J *rs=get(p->data,"resources"),*r;
        List objects=p->objects;
        select_tools(p);
        for(r=rs?rs->child:NULL;r;r=r->next){
            List args={
                0
            },dirs={
                0
            },inputs={
                0
            },defs={
                0
            };
            char label[80],*res,*source=absolute(p->directory,val(r,"path"));
            J *includes=get(r,"includeDirectories"),*defines=get(r,"defines");
            const char *value;
            Buf extra={
                0
            };
            if(!p->rc||!p->cvtres||!exists(p->rc)||!exists(p->cvtres))die("Selected resource tools were not found");
            sprintf(label,"%s_resource_%d",p->label,p->resources.n);
            res=absolute(outdir,cat(label,".res"));
            json_list(&dirs,includes?includes:get(p->data,"resourceIncludeDirectories"),"");
            extend(&dirs,&p->sdkincludes);
            json_list(&defs,defines?defines:get(p->data,"resourceDefines"),"");
            add(&args,"/nologo");
            add(&args,"/fo");
            add(&args,res);
            for(k=0;k<dirs.n;k++){
                dirs.v[k]=absolute(p->directory,dirs.v[k]);
                add(&args,"/I");
                add(&args,dirs.v[k]);
            }
            for(k=0;k<defs.n;k++){
                add(&args,"/D");
                add(&args,defs.v[k]);
                bs(&extra,defs.v[k]);
                bs(&extra,"\n");
            }
            value=setting(get(p->data,"resourceSettings"),get(r,"settings"),"Culture");
            if(*value){
                add(&args,"/l");
                add(&args,value);
            }
            value=setting(get(p->data,"resourceSettings"),get(r,"settings"),"CodePage");
            if(*value){
                add(&args,"/c");
                add(&args,value);
            }
            add(&args,source);
            distinct(&dirs,directory(source));
            if(!cache_valid(res,build_key(p->rc,&args,p->directory))){
                resource_references(&inputs,source,&dirs,extra.s);
                cached_tool(p->rc,&args,p->directory,label,res,&inputs,&dirs,NULL,1);
            }
            add(&p->resources,res);
        }
        if(!strcmp(val(p->data,"kind"),"StaticLibrary")){
            if(p->resources.n){
                List a={
                    0
                },d={
                    0
                },in={
                    0
                };
                char *obj=absolute(outdir,cat(p->label,"_resources.obj")),*converted=absolute(outdir,cat(p->label,"_resources.o"));
                add(&a,"/NOLOGO");
                add(&a,"/MACHINE:X64");
                add(&a,cat("/OUT:",obj));
                extend(&a,&p->resources);
                cached_tool(p->cvtres,&a,p->directory,cat(p->label,"_resources"),obj,&p->resources,&d,NULL,1);
                a.n=0;
                add(&a,"-r");
                add(&a,obj);
                add(&a,"-o");
                add(&a,converted);
                add(&in,obj);
                cached_tool(compiler,&a,p->directory,cat(p->label,"_resource_object"),converted,&in,&d,NULL,0);
                objects.v=NULL;
                objects.n=objects.cap=0;
                extend(&objects,&p->objects);
                add(&objects,converted);
            }
            {
                List a={
                    0
                },d={
                    0
                };
                p->archive=absolute(outdir,cat(cat(p->label,"_"),cat(val(p->data,"targetName"),".lib")));
                add(&a,"-ar");
                add(&a,"rcs");
                add(&a,p->archive);
                extend(&a,&objects);
                if(cached_tool(compiler,&a,p->directory,cat(p->label,"_archive"),p->archive,&objects,&d,NULL,0))
                    printf("Library %s.lib\n",val(p->data,"targetName"));
            }
        }
        else if(strcmp(val(p->data,"kind"),"Application"))die("Unsupported project kind");
    }
}
static List link_arguments(void) {
    List args={
        0
    },resources={
        0
    },libraries={
        0
    },dirs={
        0
    };
    int i,k;
    char *converter=NULL;
    for(i=0;i<linked_indices.n;i++){
        Project *p=&projects[atoi(linked_indices.v[i])];
        J *a,*v;
        if(p->archive&&!p->useobjects)add(&args,p->archive);
        else extend(&args,&p->objects);
        if(p==app||p->useobjects)extend(&resources,&p->resources);
        if(p->cvtres)converter=p->cvtres;
        for(k=0;k<p->libraries.n;k++)distinct(&dirs,p->libraries.v[k]);
        a=get(p->data,"libraryDirectories");
        for(v=a?a->child:NULL;v;v=v->next)distinct(&dirs,v->s);
        a=get(p->data,"linkLibraries");
        for(v=a?a->child:NULL;v;v=v->next)distinct(&libraries,v->s);
    }
    if(resources.n){
        List a={
            0
        },d={
            0
        };
        char *output=absolute(outdir,"linked_resources.obj");
        if(!converter)die("Resource converter is missing");
        add(&a,"/NOLOGO");
        add(&a,"/MACHINE:X64");
        add(&a,cat("/OUT:",output));
        extend(&a,&resources);
        cached_tool(converter,&a,app->directory,"linked_resources",output,&resources,&d,NULL,1);
        add(&args,output);
    }
    for(i=0;i<dirs.n;i++)add(&args,cat("-L",dirs.v[i]));
    for(i=0;i<libraries.n;i++){
        char *s=str(libraries.v[i]),*ext;
        if(strchr(s,'/')||strchr(s,'\\'))add(&args,s);
        else{
            ext=strrchr(s,'.');
            if(ext)*ext=0;
            add(&args,cat("-l",s));
        }
    }
    if(*val(app->data,"subsystem")){
        char *s=str(val(app->data,"subsystem"));
        _strlwr(s);
        add(&args,cat("-Wl,-subsystem=",s));
    }
    if(*val(app->data,"entryPoint"))add(&args,cat("-Wl,-e=",val(app->data,"entryPoint")));
    add(&args,"-o");
    add(&args,exepath);
    add(&args,"-MD");
    add(&args,"-MF");
    add(&args,absolute(outdir,"link.d"));
    return args;
}
static void snapshot(const char *path,List *link,List *build_inputs) {
    FILE *f;
    unsigned i,count=0;
    Entry *e;
    entry(manifestpath);
    entry(compiler);
    entry(self);
    for(i=0;i<(unsigned)build_inputs->n;i++)entry(build_inputs->v[i]);
    for(i=0;i<(unsigned)project_count;i++)if(*val(projects[i].data,"projectFile"))entry(val(projects[i].data,"projectFile"));
    for(i=0;i<8192;i++)for(e=files[i];e;e=e->next){
        Stamp now=stat_path(e->path);
        if(e->stamp.kind==1&&now.time!=e->stamp.time){
            U64 old=e->names;
            int scanned=e->scanned;
            e->scanned=0;
            if(!scanned||directory_names(e)==old)e->stamp=now;
        }
        count++;
    }
    f=file_open(path,L"wb");
    if(!f)die("Cannot write build snapshot");
    fwrite("CPCCHK02",1,8,f);
    string_write(f,exepath);
    string_write(f,compiler);
    string_write(f,command_line(compiler,link,0));
    string_write(f,app->directory);
    u32(f,timeout_seconds);
    u32(f,job_count);
    u32(f,sizeof(environment)/sizeof(environment[0]));
    for(i=0;i<sizeof(environment)/sizeof(environment[0]);i++){
        string_write(f,environment[i]);
        string_write(f,env_value(environment[i]));
    }
    u32(f,count);
    for(i=0;i<8192;i++)for(e=files[i];e;e=e->next){
        string_write(f,e->path);
        u32(f,!_stricmp(e->path,exepath)?3:e->stamp.kind);
        u64(f,e->stamp.time);
        u64(f,e->stamp.kind==1?0:e->stamp.size);
    }
    if(fclose(f))die("Cannot finish build snapshot");
}
static int build_succeeded;
static void failure_cleanup(void) {
    if(!build_succeeded&&exepath&&!skiplink){
        wchar_t *w=wide(exepath);
        DeleteFileW(w);
        free(w);
        w=wide(state_path(exepath));
        DeleteFileW(w);
        free(w);
    }
}
static void record_inputs(void) {
    J *a=jnew(JA,"");
    Buf b={
        0
    };
    int i,k;
    for(i=0;i<job_count;i++){
        J *j=jnew(JO,""),*flags=jnew(JA,""),*inputs=jnew(JA,"");
        Job *job=&jobs[i];
        set(j,"Label",job->label);
        set(j,"Source",job->source);
        set(j,"Object",job->object);
        set(j,"Directory",job->project->directory);
        set(j,"Depfile",job->dep);
        for(k=0;k<job->flags.n;k++)ja(flags,js(job->flags.v[k]));
        for(k=0;k<job->inputs.n;k++)ja(inputs,js(job->inputs.v[k]));
        put(j,"Flags",flags);
        put(j,"Inputs",inputs);
        ja(a,j);
    }
    json(&b,a);
    write_utf8(absolute(outdir,"compile_inputs.json"),b.s);
}
static int project_main(int argc,char **argv) {
    char *cwd,*projectroot=NULL,*snapshotpath,*metricspath;
    wchar_t module_w[CAP];
    List build_inputs={
        0
    },link={
        0
    };
    J *manifest,*array,*p,*metrics;
    Buf ctx={
        0
    },report={
        0
    };
    int i;
    double begin=seconds(),prepare,cache_begin,cache_seconds;
    build_start=begin;
    if(!GetModuleFileNameW(NULL,module_w,CAP))die("Cannot locate build driver");
    self=utf8(module_w);
    repo=directory(directory(self));
    if(!GetCurrentDirectoryW(CAP,module_w))die("Cannot get working directory");
    cwd=utf8(module_w);
    for(i=1;i<argc;i++){
        char *a=argv[i];
        if(!_stricmp(a,"-Rebuild"))rebuild=1;
        else if(!_stricmp(a,"-Unity"))unity=1;
        else if(!_stricmp(a,"-SkipLink"))skiplink=1;
        else if(!_stricmp(a,"-AllowWarnings"))allowwarnings=1;
        else if(!_stricmp(a,"--help")||!_stricmp(a,"-Help")){
            puts("project.exe -ProjectRoot <path> [-ManifestPath <json>] [-OutDir <path>] [-ExePath <path>] [-CompilerPath <compiler.exe>] [-BuildInputs <path> ...] [-Unity] [-UnityBatchSize <1..256>] [-Rebuild] [-SkipLink] [-CompileTimeoutSeconds <seconds>] [-AllowWarnings]\nOne compiler process at a time.");
            return 0;
        }
        else {
            if(++i>=argc)die("Missing option value");
            if(!_stricmp(a,"-CompilerPath")||!_stricmp(a,"-CpcPath"))compiler=absolute(cwd,argv[i]);
            else if(!_stricmp(a,"-ProjectRoot"))projectroot=absolute(cwd,argv[i]);
            else if(!_stricmp(a,"-ManifestPath"))manifestpath=absolute(cwd,argv[i]);
            else if(!_stricmp(a,"-OutDir"))outdir=absolute(cwd,argv[i]);
            else if(!_stricmp(a,"-ExePath"))exepath=absolute(cwd,argv[i]);
            else if(!_stricmp(a,"-BuildInputs")){
                do{
                    add(&build_inputs,absolute(cwd,argv[i]));
                    i++;
                }
                while(i<argc&&argv[i][0]!='-');
                i--;
            }
            else if(!_stricmp(a,"-Jobs")){
                if(strcmp(argv[i],"1"))die("Cprime builds require -Jobs 1");
            }
            else if(!_stricmp(a,"-CompileTimeoutSeconds")){
                timeout_seconds=atoi(argv[i]);
                if(timeout_seconds<1||timeout_seconds>3600)die("Invalid compiler timeout");
            }
            else if(!_stricmp(a,"-UnityBatchSize")){
                char *end;
                long size=strtol(argv[i],&end,10);
                if(!*argv[i]||*end||size<1||size>256)die("Invalid unity batch size");
                unity_batch_size=(int)size;
            }
            else if(!_stricmp(a,"-Toolchain")){
#ifdef BUILD_PROJECT_EXTERNAL
                if(_stricmp(argv[i],"Clang"))die("The external native project driver requires Clang");
#else
                if(_stricmp(argv[i],"Prime"))die("The native build driver uses CPC only");
#endif
            }
            else die(cat("Unknown option: ",a));
        }
    }
    if(!projectroot)projectroot=absolute(cwd,".");
    if(!compiler)compiler=absolute(repo,
#ifdef BUILD_PROJECT_EXTERNAL
        "src/third-party/clang/bin/clang.exe"
#else
        "cpc.exe"
#endif
    );
    if(!outdir)outdir=absolute(projectroot,"build/prime");
    if(!manifestpath)manifestpath=absolute(projectroot,"build/manifest/Release-x64.json");
    if(!exists(compiler))die("CPC compiler not found");
    manifest=json_read(manifestpath);
    schema=atoi(val(manifest,"schemaVersion"));
    if((schema!=1&&schema!=2)||strcmp(val(manifest,"platform"),"x64"))die("Unsupported build manifest schema or platform");
    array=get(manifest,"projects");
    for(p=array?array->child:NULL;p;p=p->next)project_count++;
    projects=alloc(project_count*sizeof(Project));
    for(p=array?array->child:NULL,i=0;p;p=p->next,i++){
        char label[64];
        Project *project=&projects[i];
        project->data=p;
        project->directory=absolute(projectroot,val(p,"directory"));
        sprintf(label,"project_%04d",i);
        project->label=str(label);
        if(!strcmp(val(p,"kind"),"Application")){
            if(app)die("Manifest must select one application");
            app=project;
        }
    }
    if(!app)die("Manifest must select one application");
    if(!exepath)exepath=absolute(projectroot,schema==2?val(app->data,"targetPath"):cat("build/",cat(val(app->data,"targetName"),".exe")));
    atexit(failure_cleanup);
    mkdirs(outdir);
    mkdirs(directory(exepath));
    snapshotpath=absolute(outdir,unity?"check-unity.bin":"check-separate.bin");
    write_utf8(snapshotpath,"");
    metricspath=absolute(outdir,"build_metrics.json");
    if(!exists(metricspath))write_utf8(metricspath,"{}");
    {
        Stamp s=entry(self)->stamp;
        char stamp[100];
        sprintf(stamp,"native-v1:%llu:%llu",s.time,s.size);
        bs(&ctx,stamp);
        s=entry(compiler)->stamp;
        sprintf(stamp,"\n%llu:%llu",s.time,s.size);
        bs(&ctx,stamp);
        for(i=0;i<(int)(sizeof(environment)/sizeof(environment[0]));i++){
            bs(&ctx,"\n");
            bs(&ctx,environment[i]);
            bs(&ctx,"=");
            bs(&ctx,env_value(environment[i]));
        }
        context=ctx.s;
    }
    measurements=jnew(JA,"");
    if(schema==2){
        select_project(app);
        order_projects();
    }
    else {
        for(i=0;i<project_count;i++){
            char idx[32];
            projects[i].selected=1;
            sprintf(idx,"%d",i);
            add(&linked_indices,idx);
        }
    }
    make_jobs();
    if(!job_count)die("The manifest contains no compile sources");
    record_inputs();
    prepare=seconds()-begin;
    cache_begin=seconds();
    for(i=0;i<job_count;i++){
        List a=job_arguments(&jobs[i]);
        jobs[i].key=build_key(compiler,&a,jobs[i].project->directory);
        jobs[i].dirty=!cache_valid(jobs[i].object,jobs[i].key);
        dirty_count+=jobs[i].dirty;
    }
    cache_seconds=seconds()-cache_begin;
    printf("%d New Item%s (%d/%d)\n",dirty_count,dirty_count==1?"":"s",job_count-dirty_count,job_count);
    fflush(stdout);
    if(dirty_count&&!skiplink){
        remove_file(exepath);
        remove_file(state_path(exepath));
        refresh(exepath);
    }
    for(i=0;i<job_count;i++)if(jobs[i].dirty){
        remove_file(jobs[i].object);
        remove_file(state_path(jobs[i].object));
        remove_file(jobs[i].dep);
        refresh(jobs[i].object);
    }
    compile_jobs();
    if(skiplink){
        build_succeeded=1;
        complete();
        return 0;
    }
    if(schema==2)resources_and_archives();
    link=link_arguments();
    {
        List inputs={
            0
        },dirs={
            0
        };
        J *j=jnew(JO,""),*a=jnew(JA,"");
        Buf b={
            0
        };
        set(j,"Compiler",compiler);
        set(j,"Directory",app->directory);
        for(i=0;i<link.n;i++)ja(a,js(link.v[i]));
        put(j,"Arguments",a);
        json(&b,j);
        write_utf8(absolute(outdir,"link_inputs.json"),b.s);
        cached_tool(compiler,&link,app->directory,"link",exepath,&inputs,&dirs,absolute(outdir,"link.d"),0);
    }
    snapshot(snapshotpath,&link,&build_inputs);
    metrics=jnew(JO,"");
    set(metrics,"Compiler",compiler);
    set(metrics,"Configuration",val(manifest,"configuration"));
    number(metrics,"Jobs",1);
    number(metrics,"Sources",source_count);
    number(metrics,"TranslationUnits",job_count);
    number(metrics,"CompiledUnits",dirty_count);
    number(metrics,"SkippedUnits",job_count-dirty_count);
    put(metrics,"Unity",jnew(JB,unity?"true":"false"));
    number(metrics,"UnityBatchSize",unity_batch_size);
    number(metrics,"BuildSeconds",seconds()-begin);
    number(metrics,"CompilerSeconds",compiler_seconds);
    number(metrics,"ResourceSeconds",resource_seconds);
    number(metrics,"PrepareSeconds",prepare);
    number(metrics,"CacheSeconds",cache_seconds);
    number(metrics,"DriverSeconds",seconds()-begin-compiler_seconds-resource_seconds);
    put(metrics,"Processes",measurements);
    json(&report,metrics);
    write_utf8(metricspath,report.s);
    complete();
    build_succeeded=1;
    return 0;
}
int wmain(int argc,wchar_t **wide_args) {
    char **args=alloc(argc*sizeof(char*));
    int i;
    for(i=0;i<argc;i++)args[i]=utf8(wide_args[i]);
    return project_main(argc,args);
}
