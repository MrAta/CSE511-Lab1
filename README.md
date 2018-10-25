# Async-IO-with-signals

## Overview

This is an implementation of Asynchronous I/O using POSIX Signals. This implementation does not use calls such as select, poll or epoll. Instead it uses event notification as a way to schedule events.

## Feature highlights

* Single event driven thread to remove synchronization contention.

* Use of POSIX realtime signals to enable queueing of signals.

* Thread pool for handling file I/O, making the application entirely non-blocking while exploiting multiprocessor architecture.

* Uses an in-memory LRU write-through cache implementation.

* Various tunable knobs such as cache size, thread pool size, etc. provided as macros

## Build instructions

To build asynchronous server and test clients:

```bash
$ make
```
To clean:

```bash
$ make clean
```
To run the server:
Type 1 (Blocking socket): 
```bash
$ ./server 1
```
Type 2 (Non-Blocking socket using epoll):
```bash
$ ./server 2
```
Type 3 (Non-Blocking with UNIX signals):
```bash
$ ./server 3
```

To run the clients:

```bash
$ ./client
$ ./client2
$ ./client3
$ ./client4
```
The binary `client` contains the code necessary to run the big experiment. 