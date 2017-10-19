# Disk-Scheduler
A concurrent program to issue and service disk requests. The disk scheduler gets and schedules disk I/Os for multiple threads. Threads issue disk requests by queueing them at the disk scheduler. The disk scheduler queue can contain at most a specified number of requests; threads must wait if the queue is full.

Requests in the disk queue are NOT serviced in FIFO order but instead in SSTF order (shortest seek time first), since the latter requires less number of head movements.

The service thread keeps the disk queue as full as possible to minimize average seek distance and **only handles a request when the disk queue has the largest possible number of requests**.
