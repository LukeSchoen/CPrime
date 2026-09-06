#ifndef _CPC_THREAD_NATIVE
#define _CPC_THREAD_NATIVE
#ifdef __cplusplus
extern "C" {
#endif
void *__cpc_windows_mutex_create(int recursive);
void __cpc_windows_mutex_destroy(void *mutex);
void __cpc_windows_mutex_lock(void *mutex);
int __cpc_windows_mutex_try_lock(void *mutex);
void __cpc_windows_mutex_unlock(void *mutex);
void *__cpc_windows_condition_create(void);
void __cpc_windows_condition_destroy(void *condition);
void __cpc_windows_condition_wait(void *condition, void *mutex);
void __cpc_windows_condition_notify(void *condition, int all);
void *__cpc_windows_thread_create(void (*entry)(void *), void *argument);
void __cpc_windows_thread_join(void *thread);
void __cpc_windows_thread_detach(void *thread);
unsigned int __cpc_windows_thread_id(void);
unsigned int __cpc_windows_thread_handle_id(void *thread);
unsigned int __cpc_windows_hardware_concurrency(void);
void __cpc_windows_sleep_for_milliseconds(long long milliseconds);
long long __cpc_windows_steady_nanoseconds(void);
long long __cpc_windows_system_ticks(void);
#ifdef __cplusplus
}
#endif
#endif
