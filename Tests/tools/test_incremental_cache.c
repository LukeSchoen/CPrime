/* Exercise the production cache without compiling a large application. */
#define main project_driver_main
#define wmain project_driver_wmain
#include "../../src/tools/build_project.c"
#undef main
#undef wmain

int main(int argc, char **argv) {
    List inputs={0}, dirs={0}; char label[80], *root, *output, *header; int i;
    double begin; FILE *f;
    if(argc!=2)die("Pass a generated fixture directory under build/");
    root=absolute(".",argv[1]);mkdirs(root);output=absolute(root,"unit.obj");
    write_utf8(output,"object");
    for(i=0;i<128;i++){sprintf(label,"header%d.h",i);header=absolute(root,label);write_utf8(header,"original");add(&inputs,header);}
    for(i=0;i<192;i++){sprintf(label,"include%d",i);add(&dirs,absolute(root,label));mkdirs(dirs.v[i]);}
    cache_save(output,"command",&inputs,&dirs);
    memset(files,0,sizeof(files));begin=seconds();
    for(i=0;i<207;i++)if(!cache_valid(output,"command"))die("Valid shared dependency cache rejected");
    if(seconds()-begin>1)die("207 cached units exceeded one second");
    printf("PASS: 207 cached units in %.3fs\n",seconds()-begin);
    if(entry(output)!=entry(cat(root,"/./unit.obj")))die("Equivalent paths did not share a cache entry");
    if(cache_valid(output,"different command"))die("Changed command reused output");
    {
        List args={0};
        char *key;
        Stamp original=entry(output)->stamp;
        context="test environment";
        key=build_key(output,&args,root);
        entry(output)->stamp.time++;
        if(!strcmp(key,build_key(output,&args,root)))die("Compiler change did not invalidate command key");
        entry(output)->stamp=original;
        free(key);
    }
    write_utf8(header,"changed header contents");memset(files,0,sizeof(files));
    if(cache_valid(output,"command"))die("Changed header reused output");
    cache_save(output,"command",&inputs,&dirs);
    write_utf8(absolute(dirs.v[0],"new.h"),"new include candidate");memset(files,0,sizeof(files));
    if(cache_valid(output,"command"))die("New header reused output");
    cache_save(output,"command",&inputs,&dirs);
    write_utf8(absolute(dirs.v[0],"unrelated.log"),"log");memset(files,0,sizeof(files));
    if(!cache_valid(output,"command"))die("Unrelated output invalidated headers");
    f=file_open(state_path(output),L"wb");fwrite("CPCSTA01",1,8,f);fclose(f);
    if(cache_valid(output,"command"))die("Truncated cache accepted");
    puts("PASS: command/header/directory/corrupt-state invalidation");return 0;
}
