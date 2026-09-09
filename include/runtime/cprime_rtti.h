#ifndef CPRIME_RTTI_RUNTIME_H
#define CPRIME_RTTI_RUNTIME_H

/* CPC vtables retain their offset-to-top at [-1] and carry this descriptor
   at [-2]. Nodes describe base-subobject paths, including nonpublic bases. */
typedef struct CpcRttiNode {
    const char *name;
    long long offset;
    int parent;
    unsigned public_base;
} CpcRttiNode;

typedef struct CpcRttiTable {
    long long offset_to_top;
    unsigned count;
    unsigned reserved;
    const CpcRttiNode *nodes;
    const void *type_info;
    unsigned long long object_size;
} CpcRttiTable;

#endif
