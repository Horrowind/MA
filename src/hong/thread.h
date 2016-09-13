#ifndef BASE_THREAD_H
#define BASE_THREAD_H

#ifdef  __STDC_NO_THREADS__

#include <pthread.h>

typedef enum {
    THREAD_ERROR_NONE    = 0,
    THREAD_ERROR_BUSY    = EBUSY,
    THREAD_ERROR_NOMEM   = ENOMEM,
    THREAD_ERROR_INVALID = EINVAL,
} thread_error_t;


typedef pthread_t thread_t ;
typedef pthread_cond_t cond_t;
typedef pthread_mutex_t mutex_t;
typedef void *(*thread_sig_t)(void *);

#define THREAD_SIG(name, data) void* name(void* data)

static inline
_Noreturn int thread_exit() {
    pthread_exit(NULL);
}

static inline
thread_error_t thread_create(thread_t* id, thread_sig_t func, void* data) {
    return pthread_create(id, NULL, func, data);
}

static inline
thread_error_t thread_join(thread_t id) {
    return pthread_join(id, NULL);
}

static inline
thread_t thread_current() {
    return pthread_self();
}

static inline
int thread_equal(thread_t t1, thread_t t2) {
    return pthread_equal(t1, t2);
}

static inline
thread_error_t mutex_init(mutex_t* mutex) {
    return pthread_mutex_init(mutex, NULL);
}

static inline
thread_error_t mutex_deinit(mutex_t* mutex) {
    return pthread_mutex_destroy(mutex);
}

static inline
thread_error_t mutex_lock(mutex_t* mutex) {
    return pthread_mutex_lock(mutex);
}

static inline
thread_error_t mutex_trylock(mutex_t* mutex) {
    return pthread_mutex_trylock(mutex);
}

static inline
thread_error_t mutex_unlock(mutex_t* mutex) {
    return pthread_mutex_unlock(mutex);
}


static inline
thread_error_t cond_init(cond_t* cond) {
    return pthread_cond_init(cond, NULL);
}

static inline
thread_error_t cond_deinit(cond_t* cond) {
    return pthread_cond_destroy(cond);
}

static inline
thread_error_t cond_signal(cond_t* cond) {
    return pthread_cond_signal(cond);
}

static inline
thread_error_t cond_wait(cond_t* cond, mutex_t* mutex) {
    return pthread_cond_wait(cond, mutex);
}




#else

#include <threads.h>

typedef enum {
    THREAD_ERROR_NONE    = thrd_success,
    THREAD_ERROR_BUSY    = thrd_busy,
    THREAD_ERROR_NOMEM   = thrd_nomem,
    THREAD_ERROR_INVALID = thrd_error,
} thread_error_t;


typedef thrd_t thread_t;
typedef cnd_t cond_t;
typedef mtx_t mutex_t;
typedef thrd_start_t thread_sig_t;

#define THREAD_SIG(name, data) int name(void* data)

static inline
thread_error_t thread_create(thread_t* id, thread_sig_t func, void* data) {
    return thrd_create(id, func, data);
}

static inline
thread_error_t thread_join(thread_t id) {
    return thrd_join(id, NULL);
}

static inline
_Noreturn int thread_exit() {
    thrd_exit(0);
}

static inline
thread_t thread_current() {
    return thrd_current();
}

static inline
int thread_equal(thread_t t1, thread_t t2) {
    return thrd_equal(t1, t2);
}

static inline
thread_error_t mutex_init(mutex_t* mutex) {
    return mtx_init(mutex, mtx_plain);
}

static inline
thread_error_t mutex_deinit(mutex_t* mutex) {
    mtx_destroy(mutex);
    return THREAD_ERROR_NONE;
}

static inline
thread_error_t mutex_lock(mutex_t* mutex) {
    return mtx_lock(mutex);
}


static inline
thread_error_t mutex_trylock(mutex_t* mutex) {
    return mtx_trylock(mutex);
}

static inline
thread_error_t mutex_unlock(mutex_t* mutex) {
    return mtx_unlock(mutex);
}

static inline
thread_error_t cond_init(cond_t* cond) {
    return cnd_init(cond);
}

static inline
thread_error_t cond_deinit(cond_t* cond) {
    cnd_destroy(cond);
    return THREAD_ERROR_NONE;
}

static inline
thread_error_t cond_signal(cond_t* cond) {
    return cnd_signal(cond);
}

static inline
thread_error_t cond_wait(cond_t* cond, mutex_t* mutex) {
    return cnd_wait(cond, mutex);
}


#endif





#endif //BASE_THREAD_H
