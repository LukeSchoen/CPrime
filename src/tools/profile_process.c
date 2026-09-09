/* Sample a single process's main thread against a CPC linker map.
   Usage: profile_process map-file executable response-file */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

__declspec(dllimport) unsigned __stdcall timeBeginPeriod(unsigned);
__declspec(dllimport) unsigned __stdcall timeEndPeriod(unsigned);
typedef struct { unsigned long long address, leaf, stack; char name[256]; } Symbol;
static Symbol symbols[65536];
static int count;
static int by_address(const void *a, const void *b) {
    unsigned long long x=((const Symbol *)a)->address,y=((const Symbol *)b)->address;
    return x<y?-1:x>y;
}
static int by_count(const void *a, const void *b) {
    unsigned long long x=((const Symbol *)a)->stack,y=((const Symbol *)b)->stack;
    return x>y?-1:x<y;
}
static int lookup(unsigned long long address) {
    int lo=0,hi=count;
    while(lo<hi) { int mid=lo+(hi-lo)/2; if(symbols[mid].address<=address)lo=mid+1;else hi=mid; }
    return lo && address-symbols[lo-1].address<65536?lo-1:-1;
}
int main(int argc,char **argv) {
    FILE *map; char line[1024],command[32768];
    STARTUPINFOA si={0}; PROCESS_INFORMATION pi={0};
    unsigned long long samples=0,unknown=0; DWORD code; int i;
    if(argc!=4) { fprintf(stderr,"Usage: profile_process map-file executable response-file\n");return 2; }
    map=fopen(argv[1],"r"); if(!map)return 2;
    while(count<65536 && fgets(line,sizeof line,map))
        if(sscanf(line,"%llx %255s",&symbols[count].address,symbols[count].name)==2)count++;
    fclose(map); qsort(symbols,count,sizeof *symbols,by_address);
    snprintf(command,sizeof command,"\"%s\" @\"%s\"",argv[2],argv[3]);
    si.cb=sizeof si;
    if(!CreateProcessA(argv[2],command,NULL,NULL,FALSE,0,NULL,NULL,&si,&pi))return 2;
    timeBeginPeriod(1);
    while(WaitForSingleObject(pi.hProcess,1)==WAIT_TIMEOUT) {
        CONTEXT ctx; int seen[32],n=0,idx,depth; unsigned long long frame,pair[2]; SIZE_T bytes;
        memset(&ctx,0,sizeof ctx); ctx.ContextFlags=CONTEXT_CONTROL|CONTEXT_INTEGER;
        if(SuspendThread(pi.hThread)==(DWORD)-1)break;
        if(GetThreadContext(pi.hThread,&ctx)) {
            samples++; idx=lookup(ctx.Rip);
            if(idx>=0) { symbols[idx].leaf++;seen[n++]=idx; } else unknown++;
            frame=ctx.Rbp;
            for(depth=0;depth<24 && frame;depth++) {
                unsigned long long next;
                if(!ReadProcessMemory(pi.hProcess,(void *)frame,pair,sizeof pair,&bytes)||bytes!=sizeof pair)break;
                idx=lookup(pair[1]);
                if(idx>=0) { for(i=0;i<n && seen[i]!=idx;i++); if(i==n)seen[n++]=idx; }
                next=pair[0]; if(next<=frame || next-frame>16777216)break; frame=next;
            }
            for(i=0;i<n;i++)symbols[seen[i]].stack++;
        }
        ResumeThread(pi.hThread);
    }
    /* A sampling failure must not leave the compiler running behind the
       caller's next serial compilation. */
    WaitForSingleObject(pi.hProcess,INFINITE);
    timeEndPeriod(1); GetExitCodeProcess(pi.hProcess,&code);
    CloseHandle(pi.hThread); CloseHandle(pi.hProcess);
    qsort(symbols,count,sizeof *symbols,by_count);
    printf("Samples=%llu unmapped-leaf=%llu exit=%lu\n  stack%%    leaf%%  symbol\n",samples,unknown,code);
    for(i=0;i<count && i<80 && symbols[i].stack;i++)
        printf("%8.2f %8.2f  %s\n",100.0*symbols[i].stack/samples,100.0*symbols[i].leaf/samples,symbols[i].name);
    return code;
}
