/* Native x64 exception dispatch. Normal execution consults no runtime state:
   the compiler emits IP/state tables and calls helpers only for throw/catch. */
#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "cprime_exception.h"
#include "rtti.inc"

void __cpc_eh_deallocate_sized(void *context)
{
    CpcEhDeallocation *allocation = (CpcEhDeallocation *)context;
    allocation->function(allocation->data, allocation->size);
}

void __cpc_eh_destroy_array(void *context)
{
    CpcEhArray *array = (CpcEhArray *)context;
    while (array->count) {
        --array->count;
        array->destructor((char *)array->data + array->count * array->element_size);
    }
}

/* The bundled SDK predates this declaration, although its x64 context types
   and the operating-system entry point are present. */
NTSYSAPI PEXCEPTION_ROUTINE NTAPI RtlVirtualUnwind(DWORD, DWORD64, DWORD64,
    PRUNTIME_FUNCTION, PCONTEXT, PVOID *, PDWORD64, PKNONVOLATILE_CONTEXT_POINTERS);

#define CPC_EXCEPTION_CODE 0xe0435043u
#define CPC_EXCEPTION_UNWINDING 2u
#define CPC_EXCEPTION_EXIT_UNWIND 4u
#define CPC_EXCEPTION_TARGET_UNWIND 0x20u
#define CPC_UNW_FLAG_EHANDLER 1u

typedef struct CpcEhException {
    unsigned magic;
    volatile LONG references;
    const CpcEhType *type;
    CpcEhDestructor destructor;
    void *object;
} CpcEhException;

typedef struct CpcEhThread {
    CpcEhCatchContext *caught;
    unsigned uncaught;
    unsigned terminating;
    uintptr_t cleanup_boundary;
} CpcEhThread;

static volatile LONG cpc_eh_tls_init;
static DWORD cpc_eh_tls = FLS_OUT_OF_INDEXES;
static CpcEhTerminateHandler cpc_eh_terminate_handler;

static VOID CALLBACK cpc_eh_free_thread(PVOID state)
{
    free(state);
}

static void cpc_eh_init_thread_key(void)
{
    if (!InterlockedCompareExchange(&cpc_eh_tls_init, 1, 0)) {
        cpc_eh_tls = FlsAlloc(cpc_eh_free_thread);
        if (cpc_eh_tls == FLS_OUT_OF_INDEXES) abort();
        InterlockedExchange(&cpc_eh_tls_init, 2);
    } else {
        while (cpc_eh_tls_init != 2) SwitchToThread();
    }
}

static CpcEhThread *cpc_eh_thread(void)
{
    CpcEhThread *state;
    if (cpc_eh_tls_init != 2) cpc_eh_init_thread_key();
    state = (CpcEhThread *)FlsGetValue(cpc_eh_tls);
    if (!state) {
        state = (CpcEhThread *)calloc(1, sizeof(*state));
        if (!state || !FlsSetValue(cpc_eh_tls, state))
            abort();
    }
    return state;
}

/* An in-memory compiler image must release OS callbacks before its code is
   unmapped. FlsFree visits the remaining thread/fiber values while this
   module's callback and allocator imports are still valid. The embedding
   caller has finished executing the image before requesting its deletion. */
void __cpc_eh_shutdown(void)
{
    if (cpc_eh_tls_init == 2) {
        FlsFree(cpc_eh_tls);
        cpc_eh_tls = FLS_OUT_OF_INDEXES;
        InterlockedExchange(&cpc_eh_tls_init, 0);
    }
}

void __cpc_eh_terminate(void)
{
    CpcEhThread *thread = cpc_eh_thread();
    CpcEhTerminateHandler handler;
    if (thread->terminating) abort();
    thread->terminating = 1;
    handler = (CpcEhTerminateHandler)InterlockedCompareExchangePointer(
        (PVOID volatile *)&cpc_eh_terminate_handler, 0, 0);
    thread->cleanup_boundary = (uintptr_t)&handler;
    if (handler) handler();
    abort();
}

/* Exception-driven termination enters an implicit handler. In particular,
   current_exception and a rethrow must name the exception that violated the
   noexcept boundary, rather than an enclosing, previously caught exception. */
static void cpc_eh_terminate_exception(CpcEhException *exception)
{
    CpcEhCatchContext context = {0};
    CpcEhThread *thread = cpc_eh_thread();
    context.exception = exception;
    context.adjusted = exception->object;
    context.previous = thread->caught;
    /* Expose the active exception without claiming that dispatch reached a
       source handler: the in-flight exception count remains observable. */
    thread->caught = &context;
    __cpc_eh_terminate();
}

CpcEhTerminateHandler __cpc_eh_set_terminate(CpcEhTerminateHandler handler)
{
    return (CpcEhTerminateHandler)InterlockedExchangePointer(
        (PVOID volatile *)&cpc_eh_terminate_handler, (PVOID)handler);
}

static int cpc_eh_same_type(const CpcEhType *left, const CpcEhType *right)
{
    if (left == right) return 1;
    if (!left || !right || left->kind != right->kind) return 0;
    if (left->kind == CPC_EH_POINTER)
        return left->pointee->qualifiers == right->pointee->qualifiers
            && cpc_eh_same_type(left->pointee, right->pointee);
    return !strcmp(left->name, right->name);
}

/* Qualification conversions at depth > 1 require const at every intervening
   pointer level, preventing the unsafe T** -> const T** conversion. */
static int cpc_eh_qualification(const CpcEhType *source, const CpcEhType *target,
                                int all_intermediate_const, int depth)
{
    unsigned added;
    if (!source || !target || source->kind != target->kind) return 0;
    if (source->qualifiers & ~target->qualifiers) return 0;
    added = target->qualifiers & ~source->qualifiers;
    if (added && depth > 1 && !all_intermediate_const) return 0;
    if (source->kind == CPC_EH_POINTER)
        return cpc_eh_qualification(source->pointee, target->pointee,
            all_intermediate_const && (target->qualifiers & CPC_EH_CONST), depth + 1);
    return cpc_eh_same_type(source, target);
}

static int cpc_eh_class_adjustment(const CpcEhType *source, const CpcEhType *target,
                                  intptr_t *adjustment)
{
    unsigned i;
    if (cpc_eh_same_type(source, target)) { *adjustment = 0; return 1; }
    if (source->kind != CPC_EH_CLASS || target->kind != CPC_EH_CLASS) return 0;
    /* The compiler supplies all accessible, unambiguous bases, including
       transitive bases. Inaccessible/ambiguous bases have no entry. */
    for (i = 0; i < source->base_count; ++i)
        if (cpc_eh_same_type(source->bases[i].type, target)) {
            *adjustment = (intptr_t)source->bases[i].offset;
            return 1;
        }
    return 0;
}

static int cpc_eh_match(CpcEhException *exception, const CpcEhType *target,
                        void **adjusted, void **pointer_value)
{
    const CpcEhType *source = exception->type;
    intptr_t adjustment;
    void *pointer;
    if (!target || cpc_eh_same_type(source, target)) {
        *adjusted = exception->object;
        return 1;
    }
    if (cpc_eh_class_adjustment(source, target, &adjustment)) {
        *adjusted = (char *)exception->object + adjustment;
        return 1;
    }
    if (target->kind != CPC_EH_POINTER) return 0;
    if (source->kind == CPC_EH_NULLPTR) {
        *pointer_value = 0;
        *adjusted = pointer_value;
        return 1;
    }
    if (source->kind != CPC_EH_POINTER || !source->pointee || !target->pointee)
        return 0;
    if (source->pointee->qualifiers & ~target->pointee->qualifiers) return 0;
    pointer = *(void **)exception->object;
    if (cpc_eh_qualification(source->pointee, target->pointee, 1, 1))
        adjustment = 0;
    else if (target->pointee->kind == CPC_EH_VOID
             && source->pointee->kind != CPC_EH_FUNCTION)
        adjustment = 0;
    else if (!cpc_eh_class_adjustment(source->pointee, target->pointee, &adjustment))
        return 0;
    *pointer_value = pointer ? (char *)pointer + adjustment : 0;
    *adjusted = pointer_value;
    return 1;
}

static const CpcEhCatch *cpc_eh_select(const CpcEhFunction *function, uintptr_t image,
                                     unsigned pc, CpcEhException *exception,
                                     void **adjusted, void **pointer_value)
{
    const CpcEhCatch *catches = (const CpcEhCatch *)(image + function->catches_rva);
    const CpcEhCatch *selected = 0;
    unsigned i;
    /* Source order is preserved for a try's handlers. A nested try has the
       smaller protected range and wins over an enclosing matching handler. */
    for (i = 0; i < function->catch_count; ++i) {
        const CpcEhCatch *candidate = catches + i;
        void *candidate_adjusted, *candidate_pointer = 0;
        const CpcEhType *type;
        if (pc < candidate->begin_rva || pc >= candidate->end_rva) continue;
        if (selected && candidate->begin_rva <= selected->begin_rva
                     && candidate->end_rva >= selected->end_rva) continue;
        type = candidate->type_rva ? (const CpcEhType *)(image + candidate->type_rva) : 0;
        if (cpc_eh_match(exception, type, &candidate_adjusted, &candidate_pointer)) {
            selected = candidate;
            *pointer_value = candidate_pointer;
            *adjusted = candidate_adjusted == &candidate_pointer ? pointer_value : candidate_adjusted;
        }
    }
    return selected;
}

static CpcEhException *cpc_eh_exception_record(const EXCEPTION_RECORD *record)
{
    CpcEhException *exception;
    if (record->ExceptionCode != CPC_EXCEPTION_CODE || record->NumberParameters != 2
        || record->ExceptionInformation[0] != CPC_EH_MAGIC) return 0;
    exception = (CpcEhException *)record->ExceptionInformation[1];
    return exception && exception->magic == CPC_EH_MAGIC ? exception : 0;
}

static int cpc_eh_valid_function(const CpcEhFunction *function)
{
    return function && function->magic == CPC_EH_MAGIC && function->version == CPC_EH_VERSION;
}

/* Check for an uncaught exception before handing dispatch to Windows, so
   std::terminate semantics do not depend on the OS unhandled-exception filter.
   This is a cold path. Windows still performs dispatch and the actual unwind. */
static int cpc_eh_has_handler(CpcEhException *exception)
{
    CONTEXT context;
    UNWIND_HISTORY_TABLE history;
    CpcEhThread *thread = cpc_eh_thread();
    memset(&history, 0, sizeof(history));
    RtlCaptureContext(&context);
    while (context.Rip) {
        DWORD64 image, frame, pc = context.Rip;
        PRUNTIME_FUNCTION entry = RtlLookupFunctionEntry(pc, &image, &history);
        if (entry) {
            PVOID data;
            PEXCEPTION_ROUTINE handler = RtlVirtualUnwind(CPC_UNW_FLAG_EHANDLER,
                image, pc, entry, &context, &data, &frame, 0);
            if ((void *)handler == (void *)__cpc_eh_frame_handler) {
                const CpcEhFunction *function = (const CpcEhFunction *)data;
                void *adjusted, *pointer_value;
                if (!cpc_eh_valid_function(function)) __cpc_eh_terminate();
                if (thread->cleanup_boundary && frame >= thread->cleanup_boundary)
                    cpc_eh_terminate_exception(exception);
                if (cpc_eh_select(function, (uintptr_t)image,
                                  (unsigned)(pc - image - 1), exception, &adjusted, &pointer_value))
                    return 1;
                if (function->flags & CPC_EH_FUNCTION_NOEXCEPT) cpc_eh_terminate_exception(exception);
            }
        } else {
            context.Rip = *(DWORD64 *)context.Rsp;
            context.Rsp += sizeof(DWORD64);
        }
    }
    return 0;
}

static void cpc_eh_raise(CpcEhException *exception)
{
    ULONG_PTR arguments[2];
    ++cpc_eh_thread()->uncaught;
    if (!cpc_eh_has_handler(exception)) cpc_eh_terminate_exception(exception);
    arguments[0] = CPC_EH_MAGIC;
    arguments[1] = (ULONG_PTR)exception;
    RaiseException(CPC_EXCEPTION_CODE, EXCEPTION_NONCONTINUABLE, 2, arguments);
    __cpc_eh_terminate();
}

void *__cpc_eh_allocate(const CpcEhType *type, unsigned alignment,
                        CpcEhDestructor destructor)
{
    CpcEhException *exception;
    uintptr_t storage;
    size_t prefix = sizeof(*exception) + sizeof(exception);
    if (alignment < sizeof(void *)) alignment = sizeof(void *);
    if ((alignment & (alignment - 1)) || !type || type->size > (size_t)-1 - prefix - alignment)
        __cpc_eh_terminate();
    exception = (CpcEhException *)malloc(prefix + alignment - 1 + type->size);
    if (!exception) __cpc_eh_terminate();
    storage = ((uintptr_t)exception + prefix + alignment - 1) & ~(uintptr_t)(alignment - 1);
    exception->magic = CPC_EH_MAGIC;
    exception->references = 1;
    exception->type = type;
    exception->destructor = destructor;
    exception->object = (void *)storage;
    ((CpcEhException **)storage)[-1] = exception;
    return (void *)storage;
}

void __cpc_eh_discard(void *object)
{
    if (object) {
        CpcEhException *exception = ((CpcEhException **)object)[-1];
        exception->magic = 0;
        free(exception);
    }
}

void __cpc_eh_throw(void *object)
{
    CpcEhException *exception = object ? ((CpcEhException **)object)[-1] : 0;
    if (!exception || exception->magic != CPC_EH_MAGIC) __cpc_eh_terminate();
    cpc_eh_raise(exception);
}

void __cpc_eh_rethrow(void)
{
    CpcEhCatchContext *caught = cpc_eh_thread()->caught;
    CpcEhException *exception;
    if (!caught) __cpc_eh_terminate();
    exception = (CpcEhException *)caught->exception;
    InterlockedIncrement(&exception->references);
    cpc_eh_raise(exception);
}

void __cpc_eh_retain_exception(void *handle)
{
    CpcEhException *exception = (CpcEhException *)handle;
    if (!exception) return;
    if (exception->magic != CPC_EH_MAGIC) __cpc_eh_terminate();
    InterlockedIncrement(&exception->references);
}

void *__cpc_eh_current_exception(void)
{
    CpcEhCatchContext *caught = cpc_eh_thread()->caught;
    void *handle = caught ? caught->exception : 0;
    __cpc_eh_retain_exception(handle);
    return handle;
}

void __cpc_eh_rethrow_exception(void *handle)
{
    if (!handle) __cpc_eh_terminate();
    __cpc_eh_retain_exception(handle);
    cpc_eh_raise((CpcEhException *)handle);
}

void __cpc_eh_release_exception(void *handle)
{
    CpcEhException *exception = (CpcEhException *)handle;
    CpcEhThread *thread;
    uintptr_t previous_boundary;
    if (!exception) return;
    if (exception->magic != CPC_EH_MAGIC) __cpc_eh_terminate();
    if (InterlockedDecrement(&exception->references)) return;
    thread = cpc_eh_thread();
    previous_boundary = thread->cleanup_boundary;
    thread->cleanup_boundary = (uintptr_t)&exception;
    if (exception->destructor) exception->destructor(exception->object);
    thread->cleanup_boundary = previous_boundary;
    exception->magic = 0;
    free(exception);
}

void *__cpc_eh_begin_catch(CpcEhCatchContext *context)
{
    CpcEhThread *thread = cpc_eh_thread();
    if (!context || !context->exception || !thread->uncaught) __cpc_eh_terminate();
    context->previous = thread->caught;
    thread->caught = context;
    --thread->uncaught;
    return context->adjusted;
}

void __cpc_eh_end_catch(CpcEhCatchContext *context)
{
    CpcEhThread *thread = cpc_eh_thread();
    CpcEhException *exception;
    if (!context || thread->caught != context) __cpc_eh_terminate();
    exception = (CpcEhException *)context->exception;
    thread->caught = context->previous;
    context->exception = 0;
    __cpc_eh_release_exception(exception);
}

int __cpc_eh_uncaught_exceptions(void)
{
    return (int)cpc_eh_thread()->uncaught;
}

void *__cpc_eh_begin_catch_copy(void *frame)
{
    CpcEhThread *thread = cpc_eh_thread();
    uintptr_t previous = thread->cleanup_boundary;
    thread->cleanup_boundary = (uintptr_t)frame;
    return (void *)previous;
}

void __cpc_eh_end_catch_copy(void *previous_boundary)
{
    cpc_eh_thread()->cleanup_boundary = (uintptr_t)previous_boundary;
}

int __cpc_eh_frame_handler(void *record_ptr, void *frame, void *context_ptr, void *dispatch_ptr)
{
    EXCEPTION_RECORD *record = (EXCEPTION_RECORD *)record_ptr;
    CONTEXT *context = (CONTEXT *)context_ptr;
    DISPATCHER_CONTEXT *dispatch = (DISPATCHER_CONTEXT *)dispatch_ptr;
    const CpcEhFunction *function = (const CpcEhFunction *)dispatch->HandlerData;
    CpcEhException *exception = cpc_eh_exception_record(record);
    uintptr_t image = (uintptr_t)dispatch->ImageBase;
    unsigned pc = (unsigned)(dispatch->ControlPc - image - 1);
    CpcEhThread *thread;
    if (!exception || !cpc_eh_valid_function(function)) return ExceptionContinueSearch;
    thread = cpc_eh_thread();
    if (record->ExceptionFlags & (CPC_EXCEPTION_UNWINDING | CPC_EXCEPTION_EXIT_UNWIND)) {
        const CpcEhState *states = (const CpcEhState *)(image + function->states_rva);
        const CpcEhCleanup *cleanups = (const CpcEhCleanup *)(image + function->cleanups_rva);
        int cleanup = -1, stop = -1;
        unsigned i;
        for (i = 0; i < function->state_count; ++i)
            if (pc >= states[i].begin_rva && pc < states[i].end_rva) {
                cleanup = states[i].cleanup;
                break;
            }
        if (record->ExceptionFlags & CPC_EXCEPTION_TARGET_UNWIND) {
            const CpcEhCatch *catches = (const CpcEhCatch *)(image + function->catches_rva);
            for (i = 0; i < function->catch_count; ++i)
                if (image + catches[i].landing_rva == dispatch->TargetIp) {
                    stop = catches[i].cleanup_stop;
                    break;
                }
        }
        while (cleanup != stop && cleanup >= 0) {
            const CpcEhCleanup *action;
            void *object;
            uintptr_t previous_boundary;
            if ((unsigned)cleanup >= function->cleanup_count) __cpc_eh_terminate();
            action = cleanups + cleanup;
            if (action->active_flag_offset) {
                int *active = (int *)((char *)frame + action->active_flag_offset);
                if (!*active) {
                    cleanup = action->parent;
                    continue;
                }
                *active = 0;
            }
            object = (char *)frame + action->object_offset;
            if (action->flags & CPC_EH_CLEANUP_INDIRECT) object = *(void **)object;
            previous_boundary = thread->cleanup_boundary;
            thread->cleanup_boundary = (uintptr_t)&object;
            if (action->flags & CPC_EH_CLEANUP_END_CATCH)
                __cpc_eh_end_catch((CpcEhCatchContext *)object);
            else
                ((CpcEhDestructor)(image + action->action_rva))(object);
            thread->cleanup_boundary = previous_boundary;
            cleanup = action->parent;
        }
        if (cleanup != stop) __cpc_eh_terminate();
    } else {
        const CpcEhCatch *selected;
        void *adjusted, *pointer_value;
        if (thread->cleanup_boundary && (uintptr_t)frame >= thread->cleanup_boundary)
            cpc_eh_terminate_exception(exception);
        selected = cpc_eh_select(function, image, pc, exception, &adjusted, &pointer_value);
        if (selected) {
            CpcEhCatchContext *caught = (CpcEhCatchContext *)((char *)frame + selected->context_offset);
            caught->exception = exception;
            caught->pointer_value = pointer_value;
            caught->adjusted = adjusted == &pointer_value ? &caught->pointer_value : adjusted;
            RtlUnwindEx(frame, (void *)(image + selected->landing_rva), record, exception,
                        context, dispatch->HistoryTable);
            __cpc_eh_terminate();
        }
        if (function->flags & CPC_EH_FUNCTION_NOEXCEPT) cpc_eh_terminate_exception(exception);
    }
    return ExceptionContinueSearch;
}
