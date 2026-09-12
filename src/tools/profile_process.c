/* Sample a single process's main thread against a CPC linker map.
   Usage: profile_process map-file executable response-file [module offset]
   The optional module and offset dump the raw stack and argument registers of
   the first few samples that land there, which identifies a DLL routine whose
   symbol the map cannot supply. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

__declspec(dllimport) unsigned __stdcall timeBeginPeriod(unsigned);
__declspec(dllimport) unsigned __stdcall timeEndPeriod(unsigned);
__declspec(dllimport) void *__stdcall CreateToolhelp32Snapshot(unsigned long flags,
        unsigned long pid);
__declspec(dllimport) int __stdcall Module32First(void *snapshot, void *entry);
__declspec(dllimport) int __stdcall Module32Next(void *snapshot, void *entry);

#define MAX_MODULE_NAME32 255
typedef struct {
    unsigned long dwSize, th32ModuleID, th32ProcessID, GlblcntUsage, ProccntUsage;
    unsigned char *modBaseAddr;
    unsigned long modBaseSize;
    void *hModule;
    char szModule[MAX_MODULE_NAME32 + 1];
    char szExePath[260];
} MODULEENTRY32A;
typedef struct { unsigned long long address, leaf, stack; char name[256]; } Symbol;
static Symbol symbols[65536];
static int count;
/* Leaf samples whose address is not inside a mapped symbol: kept so the
   report can say which symbol precedes them instead of discarding them. */
static unsigned long long unknown_leaf[65536];
static unsigned unknown_floor[65536];
static int unknown_leaf_count;
/* For each unmapped leaf sample, the innermost mapped frame that called it. */
static int unknown_caller[65536];
/* Same, found by scanning the raw stack instead of the frame chain: the first
   few mapped words, kept so the report can histogram them. */
#define SCAN_WORDS 8
static unsigned short unknown_scan[65536][SCAN_WORDS];
/* Loaded module containing each unmapped leaf sample (index into modules). */
static int unknown_module[65536];
/* Histogram of symbol indices, reused by the unmapped-sample reports. */
static int hits_by_symbol[65536];

#define MAX_MODULES 128
static struct {
    unsigned long long base, size;
    char name[260];
} modules[MAX_MODULES];
static int module_count;

static void load_modules(unsigned long pid)
{
    MODULEENTRY32A entry;
    void *snapshot=CreateToolhelp32Snapshot(8 /* TH32CS_SNAPMODULE */,pid);
    if(snapshot==(void *)-1)return;
    memset(&entry,0,sizeof entry);
    entry.dwSize=sizeof entry;
    if(Module32First(snapshot,&entry)) do {
        if(module_count<MAX_MODULES) {
            modules[module_count].base=(unsigned long long)entry.modBaseAddr;
            modules[module_count].size=entry.modBaseSize;
            strncpy(modules[module_count].name,entry.szModule,259);
            modules[module_count].name[259]=0;
            module_count++;
        }
        memset(&entry,0,sizeof entry);
        entry.dwSize=sizeof entry;
    } while(Module32Next(snapshot,&entry));
    CloseHandle(snapshot);
}

static int module_index(unsigned long long address)
{
    int i;
    for(i=0;i<module_count;i++)
        if(address>=modules[i].base && address<modules[i].base+modules[i].size)
            return i;
    return -1;
}

static int by_address(const void *a, const void *b) {
    unsigned long long x=((const Symbol *)a)->address,y=((const Symbol *)b)->address;
    return x<y?-1:x>y;
}
/* Symbols stay ordered by address so earlier reports can keep their indices;
   this orders indices by sample count for the printed ranking. */
static int order[65536];
static int by_count(const void *a, const void *b) {
    unsigned long long x=symbols[*(const int *)a].stack,y=symbols[*(const int *)b].stack;
    return x>y?-1:x<y;
}
static int lookup(unsigned long long address) {
    int lo=0,hi=count;
    while(lo<hi) { int mid=lo+(hi-lo)/2; if(symbols[mid].address<=address)lo=mid+1;else hi=mid; }
    return lo && address-symbols[lo-1].address<65536?lo-1:-1;
}
/* Nearest preceding symbol with no distance limit, for reporting only. */
static int lookup_floor(unsigned long long address) {
    int lo=0,hi=count;
    while(lo<hi) { int mid=lo+(hi-lo)/2; if(symbols[mid].address<=address)lo=mid+1;else hi=mid; }
    return lo-1;
}
/* A DLL leaf function may not keep a frame pointer, so walk the raw stack for
   the first few words that land in a mapped symbol. */
static void stack_callers(HANDLE process, unsigned long long stack_pointer,
                          unsigned short *out)
{
    unsigned long long word;
    SIZE_T bytes;
    int i, n = 0;
    memset(out,0,SCAN_WORDS*sizeof *out);
    for(i=0;i<512 && n<SCAN_WORDS;i++) {
        if(!ReadProcessMemory(process,(void *)(stack_pointer+8*i),&word,
                              sizeof word,&bytes)||bytes!=sizeof word)
            break;
        if(lookup(word)>=0) out[n++] = (unsigned short)(lookup(word)+1);
    }
}
/* Print the heaviest histogram entries, clearing each one as it is reported so
   the ranking comes out without an extra sorted copy. */
static void report_histogram(unsigned long long samples, const char *title,
                             int limit)
{
    int shown, i;
    printf("%s:\n",title);
    for(shown=0;shown<limit;shown++) {
        int best=-1,best_count=0;
        for(i=0;i<count;i++)
            if(hits_by_symbol[i]>best_count) {
                best_count=hits_by_symbol[i];best=i;
            }
        if(best<0)break;
        printf("    %5.2f%%  %s\n",100.0*best_count/samples,symbols[best].name);
        hits_by_symbol[best]=0;
    }
    for(i=0;i<count;i++) hits_by_symbol[i]=0;
}
int main(int argc,char **argv) {
    FILE *map; char line[1024],command[32768];
    STARTUPINFOA si={0}; PROCESS_INFORMATION pi={0};
    unsigned long long samples=0,unknown=0; DWORD code; int i;
    /* Optional: dump the stack of the first sample inside a named module
       offset, to identify a caller that the frame chain cannot recover. */
    unsigned long long watch_base=0,watch_offset=0; const char *watch_module=NULL;
    int watch_dumps=0;
    if(argc!=4 && argc!=6) {
        fprintf(stderr,"Usage: profile_process map-file executable response-file"
                       " [module offset]\n");
        return 2;
    }
    map=fopen(argv[1],"r"); if(!map)return 2;
    while(count<65536 && fgets(line,sizeof line,map))
        if(sscanf(line,"%llx %255s",&symbols[count].address,symbols[count].name)==2)count++;
    fclose(map); qsort(symbols,count,sizeof *symbols,by_address);
    snprintf(command,sizeof command,"\"%s\" @\"%s\"",argv[2],argv[3]);
    si.cb=sizeof si;
    if(!CreateProcessA(argv[2],command,NULL,NULL,FALSE,0,NULL,NULL,&si,&pi))return 2;
    load_modules(pi.dwProcessId);
    if(argc==6) {
        int m, tries;
        watch_module=argv[4];
        sscanf(argv[5],"0x%llx",&watch_offset);
        for(tries=0;tries<200;tries++) {
            unsigned long long found=0;
            for(m=0;m<module_count;m++)
                if(!strcmp(modules[m].name,watch_module))
                    found=modules[m].base;
            if(found) { watch_base=found; break; }
            Sleep(5);
            module_count=0;
            load_modules(pi.dwProcessId);
        }
        if(!watch_base) {
            fprintf(stderr,"module '%s' not loaded\n",watch_module);
            return 2;
        }
    }
    timeBeginPeriod(1);
    while(WaitForSingleObject(pi.hProcess,1)==WAIT_TIMEOUT) {
        CONTEXT ctx; int seen[32],n=0,idx,depth,caller=-1; unsigned long long frame,pair[2]; SIZE_T bytes;
        memset(&ctx,0,sizeof ctx); ctx.ContextFlags=CONTEXT_CONTROL|CONTEXT_INTEGER;
        if(SuspendThread(pi.hThread)==(DWORD)-1)break;
        if(GetThreadContext(pi.hThread,&ctx)) {
            if(watch_base && watch_dumps<3
               && ctx.Rip>=watch_base+watch_offset
               && ctx.Rip<watch_base+watch_offset+32) {
                unsigned long long words[24]; SIZE_T got=0;
                int w;
                watch_dumps++;
                printf("stacksample rip=0x%llx rsp=0x%llx rbp=0x%llx\n",
                       (unsigned long long)ctx.Rip,(unsigned long long)ctx.Rsp,
                       (unsigned long long)ctx.Rbp);
                printf("  rcx=0x%llx rdx=0x%llx r8=0x%llx r9=0x%llx\n",
                       (unsigned long long)ctx.Rcx,(unsigned long long)ctx.Rdx,
                       (unsigned long long)ctx.R8,(unsigned long long)ctx.R9);
                {   /* Peek at whatever the first argument may point to. */
                    unsigned long long regs[4];
                    int r;
                    regs[0]=(unsigned long long)ctx.Rcx;
                    regs[1]=(unsigned long long)ctx.Rdx;
                    regs[2]=(unsigned long long)ctx.R8;
                    regs[3]=(unsigned long long)ctx.R9;
                    for(r=0;r<4;r++) {
                        char text[49]; SIZE_T got=0; int k, printable=1;
                        if(regs[r]<0x10000) continue;
                        if(!ReadProcessMemory(pi.hProcess,(void *)regs[r],text,48,&got)
                           || !got) continue;
                        text[got]=0;
                        for(k=0;k<(int)got;k++) {
                            unsigned char c=(unsigned char)text[k];
                            if(c<32||c>126) printable=0;
                        }
                        printf("  arg%d \"%.*s\"%s\n",r,(int)got,text,
                               printable?"":" (non-printable)");
                    }
                }
                ReadProcessMemory(pi.hProcess,(void *)ctx.Rsp,words,sizeof words,&got);
                for(w=0;w<(int)(got/sizeof words[0]);w++)
                    printf("  rsp+%02x 0x%llx\n",w*8,words[w]);
                fflush(stdout);
            }
            samples++; idx=lookup(ctx.Rip);
            if(idx>=0) { symbols[idx].leaf++;seen[n++]=idx; }
            else {
                unknown++;
                if(unknown_leaf_count<65536) {
                    unknown_leaf[unknown_leaf_count]=ctx.Rip;
                    unknown_module[unknown_leaf_count]=module_index(ctx.Rip);
                    unknown_leaf_count++;
                }
            }
            frame=ctx.Rbp;
            for(depth=0;depth<24 && frame;depth++) {
                unsigned long long next;
                if(!ReadProcessMemory(pi.hProcess,(void *)frame,pair,sizeof pair,&bytes)||bytes!=sizeof pair)break;
                idx=lookup(pair[1]);
                if(idx>=0) {
                    if(caller<0)caller=idx;
                    for(i=0;i<n && seen[i]!=idx;i++); if(i==n)seen[n++]=idx;
                }
                next=pair[0]; if(next<=frame || next-frame>16777216)break; frame=next;
            }
            if(unknown_leaf_count && unknown_leaf[unknown_leaf_count-1]==ctx.Rip)
            {
                unknown_caller[unknown_leaf_count-1]=caller;
                stack_callers(pi.hProcess,ctx.Rsp,
                              unknown_scan[unknown_leaf_count-1]);
            }
            for(i=0;i<n;i++)symbols[seen[i]].stack++;
        }
        ResumeThread(pi.hThread);
    }
    module_count=0;
    load_modules(pi.dwProcessId);
    /* A sampling failure must not leave the compiler running behind the
       caller's next serial compilation. */
    WaitForSingleObject(pi.hProcess,INFINITE);
    timeEndPeriod(1); GetExitCodeProcess(pi.hProcess,&code);
    CloseHandle(pi.hThread);
    /* Distance-limited lookup leaves samples in oversized functions and in
       linker padding unattributed.  Charge them to the nearest preceding
       symbol, while the table is still ordered by address, so the report can
       say which symbol (or gap) they belong to. */
    for(i=0;i<unknown_leaf_count;i++)
        unknown_floor[i]=(unsigned)lookup_floor(unknown_leaf[i]);
    for(i=0;i<count;i++) order[i]=i;
    qsort(order,count,sizeof *order,by_count);
    printf("Samples=%llu unmapped-leaf=%llu exit=%lu\n  stack%%    leaf%%  symbol\n",samples,unknown,code);
    for(i=0;i<count && i<80 && symbols[order[i]].stack;i++)
        printf("%8.2f %8.2f  %s\n",100.0*symbols[order[i]].stack/samples,
               100.0*symbols[order[i]].leaf/samples,symbols[order[i]].name);
    if(unknown_leaf_count) {
        int shown;
        for(i=0;i<unknown_leaf_count;i++)
            if(unknown_caller[i]>=0) hits_by_symbol[unknown_caller[i]]++;
        report_histogram(samples,"unmapped leaf charged to the calling symbol",12);
        for(i=0;i<unknown_leaf_count;i++) {
            int k;
            for(k=0;k<SCAN_WORDS;k++)
                if(unknown_scan[i][k]) hits_by_symbol[unknown_scan[i][k]-1]++;
        }
        report_histogram(samples,"mapped words seen on the stack of "
                                 "unmapped leaf samples",14);
        printf("unmapped leaf addresses (16-byte buckets):\n");
        for(shown=0;shown<10;shown++) {
            int best=-1,best_count=0,j;
            unsigned long long best_base=0;
            int best_module=-1;
            for(j=0;j<unknown_leaf_count;j++) {
                unsigned long long base=unknown_leaf[j]&~0xfULL;
                int hits=0,k;
                for(k=0;k<unknown_leaf_count;k++)
                    if((unknown_leaf[k]&~0xfULL)==base)hits++;
                if(hits>best_count) { best_count=hits;best=j;best_base=base;
                                      best_module=unknown_module[j]; }
            }
            if(best<0)break;
            printf("    %5.2f%%  0x%llx  %s+0x%llx\n",100.0*best_count/samples,
                   best_base,best_module>=0?modules[best_module].name:"?",
                   best_module>=0?best_base-modules[best_module].base:best_base);
            { int w=0; for(j=0;j<unknown_leaf_count;j++)
                if((unknown_leaf[j]&~0xfULL)!=best_base) {
                    unknown_leaf[w]=unknown_leaf[j];
                    unknown_floor[w]=unknown_floor[j];
                    unknown_caller[w]=unknown_caller[j];
                    unknown_module[w]=unknown_module[j];
                    w++;
                }
              unknown_leaf_count=w; }
        }
    }
    CloseHandle(pi.hProcess);
    return code;
}
