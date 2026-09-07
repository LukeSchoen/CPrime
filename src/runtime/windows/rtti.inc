#include "cprime_rtti.h"

static int cpc_rtti_public_path(const CpcRttiTable *table, int from, int to)
{
    while (from != to && from >= 0) {
        if (!table->nodes[from].public_base) return 0;
        from = table->nodes[from].parent;
    }
    return from == to;
}

void *__cpc_dynamic_cast(void *object, long long vptr_offset,
                         const char *source, const char *target)
{
    const void *const *vtable;
    const CpcRttiTable *table;
    char *complete;
    long long source_offset, candidate = 0;
    unsigned i, j;
    int found = 0, source_public = 0, target_public = 0;
    if (!object) return NULL;
    vtable = *(const void *const **)((char *)object + vptr_offset);
    table = (const CpcRttiTable *)vtable[-2];
    complete = (char *)object + vptr_offset + table->offset_to_top;
    if (!target) return complete;
    source_offset = (char *)object - complete;

    /* A downcast selects the unique target object of which the actual source
       subobject is a public base. Repeated paths to a shared virtual base or
       target do not create extra objects. */
    for (i = 0; i < table->count; ++i) {
        const CpcRttiNode *s = table->nodes + i;
        if (s->offset != source_offset || strcmp(s->name, source)) continue;
        if (cpc_rtti_public_path(table, (int)i, 0)) source_public = 1;
        for (j = 0; j < table->count; ++j) {
            const CpcRttiNode *t = table->nodes + j;
            if (strcmp(t->name, target) || !cpc_rtti_public_path(table, (int)i, (int)j)) continue;
            if (found && candidate != t->offset) return NULL;
            candidate = t->offset;
            found = 1;
        }
    }
    if (found) return complete + candidate;

    /* Otherwise a crosscast requires a public source and a unique public
       target in the most-derived object. Inaccessible duplicates still make
       the target ambiguous. */
    if (!source_public) return NULL;
    for (i = 0; i < table->count; ++i) {
        const CpcRttiNode *t = table->nodes + i;
        if (strcmp(t->name, target)) continue;
        if (found && candidate != t->offset) return NULL;
        candidate = t->offset;
        found = 1;
        target_public |= cpc_rtti_public_path(table, (int)i, 0);
    }
    return found && target_public ? complete + candidate : NULL;
}
