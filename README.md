# Thread Library
A user space thread library in C++ that allows for multi-threaded programs to run

## Thread Library Interfaces ##

__int thread_libinit(thread_startfunc_t func, void *arg):__ thread_libinit initializes the thread library. A user program should call thread_libinit exactly once (before calling any other thread functions). thread_libinit creates and runs the first thread.  This first thread is initialized to call the function pointed to by func with the single argument arg.

__int thread_create(thread_startfunc_t func, void *arg):__ thread_create is used to create a new thread.  When the newly created thread starts, it will call the function pointed to by func and pass it the single argument arg.

__int thread_yield(void):__ thread_yield causes the current thread to yield the CPU to the next runnable thread.  It has no effect if there are no other runnable threads. thread_yield is used to test the thread library.  A normal concurrent program should not depend on thread_yield; nor should a normal concurrent program produce incorrect answers if thread_yield calls are inserted arbitrarily.

`__int thread_lock(unsigned int lock)
int thread_unlock(unsigned int lock)
int thread_wait(unsigned int lock, unsigned int cond)
int thread_signal(unsigned int lock, unsigned int cond)
int thread_broadcast(unsigned int lock, unsigned int cond)__` thread_lock, thread_unlock, thread_wait, thread_signal, and thread_broadcast implement [Mesa](https://en.wikipedia.org/wiki/Monitor_(synchronization)) monitors in your thread library.

__Note:__ Each of these functions returns 0 on success and -1 on failure except for thread_libinit, which does not return at all on success

# Disk-Scheduler
A concurrent program to issue and service disk requests. The disk scheduler gets and schedules disk I/Os for multiple threads. Threads issue disk requests by queueing them at the disk scheduler. The disk scheduler queue can contain at most a specified number of requests; threads must wait if the queue is full.

Requests in the disk queue are NOT serviced in FIFO order but instead in SSTF order (shortest seek time first), since the latter requires less number of head movements.

The service thread keeps the disk queue as full as possible to minimize average seek distance and **only handles a request when the disk queue has the largest possible number of requests**. Hence disk size is the condition variable.
