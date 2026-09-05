#ifndef CPRIME_EXCEPTION_RUNTIME_H
#define CPRIME_EXCEPTION_RUNTIME_H

/* Compiler/runtime ABI. Function tables use PE image-relative addresses.
   Type descriptors use ordinary pointers so they can be shared across TUs. */

#ifdef __cplusplus
extern "C" {
#endif

#define CPC_EH_MAGIC 0x45435043u
#define CPC_EH_VERSION 2u
#define CPC_EH_FUNCTION_NOEXCEPT 1u
#define CPC_EH_CLEANUP_INDIRECT 1u
#define CPC_EH_CLEANUP_END_CATCH 2u
#define CPC_EH_CONST 1u
#define CPC_EH_VOLATILE 2u
#define CPC_EH_SCALAR 0u
#define CPC_EH_CLASS 1u
#define CPC_EH_POINTER 2u
#define CPC_EH_NULLPTR 3u
#define CPC_EH_VOID 4u
#define CPC_EH_FUNCTION 5u

typedef struct CpcEhType CpcEhType;
typedef struct CpcEhBase {
    const CpcEhType *type;
    long long offset;
} CpcEhBase;

struct CpcEhType {
    const char *name;
    const CpcEhType *pointee;
    const CpcEhBase *bases;
    unsigned size;
    unsigned kind;
    unsigned qualifiers;
    unsigned base_count;
};

typedef struct CpcEhState {
    unsigned begin_rva;
    unsigned end_rva;
    int cleanup;
} CpcEhState;

typedef struct CpcEhCleanup {
    int parent;
    unsigned action_rva;
    int object_offset;
    unsigned flags;
    int active_flag_offset; /* zero means unconditional; otherwise a frame int */
} CpcEhCleanup;

typedef struct CpcEhCatch {
    unsigned begin_rva;
    unsigned end_rva;
    unsigned landing_rva;
    unsigned type_rva; /* zero means catch (...) */
    int cleanup_stop;
    int context_offset;
    unsigned flags;
    unsigned reserved;
} CpcEhCatch;

typedef struct CpcEhFunction {
    unsigned magic;
    unsigned version;
    unsigned flags;
    unsigned state_count;
    unsigned states_rva;
    unsigned cleanup_count;
    unsigned cleanups_rva;
    unsigned catch_count;
    unsigned catches_rva;
} CpcEhFunction;

typedef struct CpcEhCatchContext {
    struct CpcEhCatchContext *previous;
    void *exception;
    void *adjusted;
    void *pointer_value;
} CpcEhCatchContext;

typedef void (*CpcEhDestructor)(void *);
typedef void (*CpcEhTerminateHandler)(void);
typedef struct CpcEhArray {
    void *data;
    unsigned long long count;
    unsigned long long element_size;
    CpcEhDestructor destructor;
} CpcEhArray;

void __cpc_eh_destroy_array(void *context);
void __cpc_eh_shutdown(void);

void *__cpc_eh_allocate(const CpcEhType *type, unsigned alignment,
                        CpcEhDestructor destructor);
void __cpc_eh_discard(void *object);
void __cpc_eh_throw(void *object);
void __cpc_eh_rethrow(void);
void *__cpc_eh_current_exception(void);
void __cpc_eh_retain_exception(void *handle);
void __cpc_eh_release_exception(void *handle);
void __cpc_eh_rethrow_exception(void *handle);
void *__cpc_eh_begin_catch(CpcEhCatchContext *context);
void __cpc_eh_end_catch(CpcEhCatchContext *context);
void *__cpc_eh_begin_catch_copy(void *frame);
void __cpc_eh_end_catch_copy(void *previous_boundary);
int __cpc_eh_uncaught_exceptions(void);
void __cpc_eh_terminate(void);
CpcEhTerminateHandler __cpc_eh_set_terminate(CpcEhTerminateHandler handler);

/* Four Windows language-handler ABI arguments; EXCEPTION_DISPOSITION result. */
int __cpc_eh_frame_handler(void *record, void *frame, void *context, void *dispatch);

#ifdef __cplusplus
}
#endif
#endif
