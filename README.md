# Disk-Scheduler
A concurrent program to issue and service disk requests. The disk scheduler in an operating system gets and schedules disk I/Os for multiple threads. Threads issue disk requests by queueing them at the disk scheduler. The disk scheduler queue can contain at most a specified number of requests; threads must wait if the queue is full.
