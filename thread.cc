#include <cstdlib>
#include <ucontext.h>
#include <deque>
#include <iterator>
#include <map>
#include <iostream>
#include "interrupt.h"
#include "thread.h"

using namespace std;

struct Thread {
	unsigned int id;
	char* stack;
	ucontext_t* ucontext_ptr;
	bool finished;
};

struct Lock {
	Thread* owner;
	deque<Thread*>* blocked_threads;
};

typedef map<unsigned int, deque<Thread*>*>::const_iterator condition_iterator; //ability to iterate over the queue mapping cond var and its queue
typedef map<unsigned int, Lock*>::const_iterator lock_iterator;  //ability to iterate over the queue mapping lock and its queue
typedef void (*thread_startfunc_t) (void *);

static Thread* current; //the current thread that is being processed
static ucontext_t* scheduler;
static deque<Thread*> ready; //ready queue for threads in ready state

static map<unsigned int, deque<Thread*>*> conditon_map; //creating mapping between conditional variables and their queues
static map<unsigned int, deque<Lock*> lock_map; //creating mapping between map and their queues

static bool init = false; //check whether the thread is initialised
static int id = 0;

void delete_current_thread() {
	delete current->stack;
	current->ucontext_ptr->uc_stack.ss_sp = NULL:
	current->ucontext_ptr->uc_stack.ss_size = 0;
	current->ucontext_ptr->uc_stack.ss_flags = 0;
	current->ucontext_ptr->uc_link = NULL;
	delete current->ucontext_ptr;
	delete current;
	current = NULL;
}

int thread_libinit(thread_startfunc_t func, void *arg) {
	if (init) return -1; //if already initialised, error

	init = true;

	if (thread_create(func, arg) < 0)
		return -1;

	Thread* first = ready.front();
	ready.pop_front();
	current = first;

	try {
		scheduler = new ucontext_t; //scheduler is newly created context
	} catch (std::bad_alloc b) { //if a bad memory allocation occurs
		delete scheduler;
		return -1;
	}
	getcontext(scheduler);

	//switch to current thread
	interrupt_disable();
	swapcontext(scheduler, first->ucontext_ptr); //swap from new to current = first context, since can't switch directly

	while (read.size() > 0) { //if threads are in ready queue
		if (current->finished == true) //if a thread is finished processing, delete it
			delete_current_thread();
		Thread* next = ready.front(); // repeat it for every thread in queue
		read.pop_front();
		current = next;
		swapcontext(scheduler, current->ucontext_ptr);
	}

	if (current != NULL) //if still some thread is left in the end, delete it
		delete_current_thread();

	//All threads have executes, exit library
	cout<< "Exiting Thread library.\n";
	exit(0);
	interrupt_enable();
}

static int execute_func (thread_startfunc_t func, void *arg) { //execute the function the thread wants
	interrupt_enable(); //interrupts are for management of queues only
	func(arg);
	interrupt_disable();

	current->finished = true; //mark thread as completed
	swapcontext(current->ucontext_ptr, scheduler); //transfer the current context to scheduler
	return 0;
}

int thread_create(thread_startfunc_t func, void *arg) {
	if (!init)
		return -1; //if not initialised, error

	interrupt_disable();
	Thread* t;
	try {
		t = new Thread;
		t->ucontext_ptr = new ucontext_t;
		getcontext(t->ucontext_ptr);

		t->stack = new char [STACK_SIZE];
		current->ucontext_ptr->uc_stack.ss_sp = t->stack:
		current->ucontext_ptr->uc_stack.ss_size = STACK_SIZE;
		current->ucontext_ptr->uc_stack.ss_flags = 0;
		current->ucontext_ptr->uc_link = NULL;
		//makecontext only works on a new context with the above configs 
		makecontext(t->ucontext_ptr, (void (*)())execute_func, 2, func, arg);

		t->id = id;
		id++;
		t->finished = false;
		ready.push_back(t); //push back to the ready queue with id=id
	} catch (std::bad_alloc b) { //bad memory allocation 
		delete t->ucontext_ptr;
		delete t->stack;
		delete t;
		interrupt_enable();
		return -1;
	}

	interrupt_enable();
	return 0;
}

int thread_yield(void) { //giving up the CPU
	if (!init)
		return -1;

	interrupt_disable();
	ready.push_back(current); //push back to the ready queue and swap context with scheduler
	swapcontext(current->ucontext_ptr, scheduler);
	interrupt_enable();
	return 0;
}

int thread_lock(unsigned int lock) {
	if (!init)
		return -1;

	interrupt_disable();
	lock_iterator lock_iter = lock_map.find(lock); //find lock in thread
	Lock* l;
	if (lock_iter == lock_map.end()) { //New lock goes to the end
		try {
			l = new Lock;
			l->owner = current;
			l->blocked_threads = new deque<Thread*>; //create a blocked queue for the other threads trying to access this lock
		} catch (std:bad_alloc b) {
			delete l->blocked_threads;
			delete l;
			interrupt_enable();
			return -1;
		}
		lock_map.insert(make_pair(lock,l)); //made pair of lock and l which information has info of lock
	}
	else { //lock is found in the map
		l = (*lock_iter).second; //lock info is in l

		if (l->owner == NULL) //lock is free, give to the thread asking
			l->owner = current;

		else { //lock has a owner
			if (l->owner->id == current->id) { //if the owner is the thread asking
				interrupt_enable();
				return -1;
			}
			else { //lock is owned by another thread
				//put it in block thread queue of the thread and swap context
				l->blocked_threads->push_back(current);
				swapcontext(current->ucontext_ptr, scheduler);
			}
		}
	}
	interrupt_enable();
	return 0;
}

int unlock_without_interrupts(unsigned int lock) {
	lock_iterator lock_iter = lock_map.find(lock);
	Lock* l;

	if (lock_iter == lock_map.end()) //new lock, can't be unlocked if unacquired
		return -1;

	else { //lock found
		l = (*lock_iter).second; //lock info is in l

		if (l->owner == NULL) //lock is free, can't unlock
			return -1;

		else { //lock has a owner
			if (l->owner == current->id) { //current thread owns the lock
				if (l->blocked_threads->size() > 0) {
					l->owner = l->blocked_threads->front(); //give the lock to next in line
					l->blocked_threads->pop_front();
					ready.push_back(l->owner); //remove the new thread from blocked queue and put it in ready queue
				}
				else
					l->owner = NULL; //no thread waiting, hence free lock	
			}
			else //current thread is not the owner
				return -1;
		}
	}
	return 0;
}

int thread_unlock(unsigned int lock) {
	if (!init)
		return -1;

	interrupt_disable();
	int result = unlock_without_interrupts(lock);
	interrupt_enable();
	return result;
}

int thread_wait(unsigned int lock, unsigned int cond) {
	if (!init)
		return -1;

	interrupt_disable();
	if (unlock_without_interrupts(lock) == 0) { //if unlock of the lock is successfull
		condition_iterator cond_iter = conditon_map.find(cond);

		if (cond_iter == conditon_map.end()) { //if its a new conditional variable
			deque<Thread*>* waiting_threads; // create a new queue will all other threads will wait

			try {
				waiting_threads = new deque<Thread*>;
			} catch (std::bad_alloc b) {
				delete waiting_threads;
				interrupt_enable();
				return -1;
			}

			waiting_threads->push_back(current); //push the current thread in the queue
			conditon_map.insert(make_pair(cond,waiting_threads)); //create a mapping between the cond variable and its queue
		}

		else //found the cond variable
			(*cond_iter).second->push_back(current); //push back the current thread to the queue of the cond

		swapcontext(current->ucontext_ptr,scheduler); //switch context
		interrupt_enable();
		return thread_lock(lock);
	}
	interrupt_enable();
	return 0;
}

int thread_signal(unsigned int lock, unsigned int cond) {
	if (!init)
		return -1;

	interrupt_disable();
	condition_iterator cond_iter = conditon_map.find(cond);

	if (cond_iter != conditon_map.end()) { //condition found
		deque<Thread*>* cq = (*cond_iter).second; //queue associated with the cond variable
		
		if (!cq->empty()) { //threads are waiting
			Thread* t = cq->front();
			cq->pop_front();
			ready.push_back(t); //pop the next waiting queue and push it back in ready queue
		}
	}
	//if cond is not found, it is not considered an error
	interrupt_enable();
	return 0;
}

int thread_broadcast(unsigned int lock, unsigned int cond) {
	if (!init)
		return -1;

	interrupt_disable();
	condition_iterator cond_iter = conditon_map.find(cond);

	if (cond_iter != conditon_map.end()) { //condition found
		deque<Thread*>* cq = (*cond_iter).second; //queue associated with the cond variable
		
		while (cq->size() > 0) { //until threads are not waiting
			Thread* t = cq->front();
			cq->pop_front();
			ready.push_back(t); //pop the next waiting queue and push it back in ready queue
		}
	}
	//if cond is not found, it is not considered an error
	interrupt_enable();
	return 0;
}
