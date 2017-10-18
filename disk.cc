#include "thread.h"
#include "interrupt.h"
#include <sctdlib>
#include <deque>
#include <fstream>
#include <algorithm>
#include <iostream>
#include <stdio.h>

using namespace std;

struct request {
	int id;
	int track;
};
//disk scheduler uses Shortest Seek Time First (SSTF) Algorithm. here variable track is synonyms with head
int mutex, diskQfull, diskQopen, diskQmax, numthreads, lasttrack; //lastrack is the disk just being serviced
char** filenames;
bool* serviced;
deque<request*> diskQ;

bool mysort(request *i, request *j) {
	return (abs(i->track - lasttrack) < abs(j->track - lasttrack)); //sorting the disk queue so that the disk requested disk after the lasttrack is requested
}

bool isdiskQfull() {
	return diskQ.size() >= diskQmax;
}

void sendRequest(request *r) { //appending the request to the disk queue
	cout << "sending request with id " << r->id << "and track " << r->track << endl;
	diskQ.push_back(r);
	sort(diskQ.begin(), diskQ.end(), mysort);
	serviced[r->id] = false; //by default the request is not serviced.
}

void serviceRequest() { //servicing the request at the front of the queue
	request *r = diskQ.front();
	diskQ.pop_front();
	cout << "now servicing request with id " << r->id << "and track " << r->track <<endl;
	serviced[r->id] = true; //indicating that the request is serviced.
	lasttrack = r->track; //updating the lasttrack with the latest serviced track
	delete r;
}

void scheduler(void* (int)id) { //schedules the requests to ensure synchronisation
	int requesterID = id;
	ifstream in(filenames[requesterID]);
	int track = 0;

	while (true) {
		if (in.eof()) //if in reach end of file, break out of the loop
			break;

	r = new request;
	r->id = requesterID;
	r->track = track;
	thread_lock(mutex); //locking the resources with mutex

	while (isdiskQfull() || serviced[r->id] == false) //MESA
		thread_wait(mutex, diskQfull);
	}
	in.close();

	while (serviced[requesterID] == false)
		thread_wait(mutex, diskQfull);

	//after thread is serviced, decrease the number of threads
	numthreads--;
	if (numthreads < diskQmax) {
		diskQmax--;
		thread_broadcast(mutex, diskQfull);
	}
	thread_unlock(mutex);
}

void service(void* arg) {
	thread_lock(mutex);
	while (diskQmax > 0) {
		while (!isdiskQfull()) //MESA, use "if" for hoare
			thread_wait(mutex, diskQfull);

		if (!diskQ.empty())
			serviceRequest();

		thread_broadcast(mutex, diskQopen);
	}
	thread_unlock(mutex);
}

void master(void* arg) {
	thread_create(service, NULL); //to create the main thread

	for (int i = 0; i < numthreads; i++)
		thread_create(scheduler, (void*)i); //to create threads for each file
}

int main(int argc, char** argv) {
	if (argc <=2)
		return 0;

	diskQmax = atoi(argv[1]); //second input is the number of disks
	numthreads = argc - 2; //#threads = total threads - command - #disks
	lasttrack =  0;
	mutex = 1;
	diskQfull = 2;
	diskQopen = 3;

	serviced = new bool[numthreads];
	for (int i = 0; i < numthreads; ++i)
		serviced[i] = true; //all the threads and requests are serviced by default. this will change when they are added to the queue

	filenames = argv + 2;
	thread_libinit(&master, NULL);

	delete serviced;
	return 0;
}	
