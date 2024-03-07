# Group18_P2
# [Elevator]

[This project is designed to provide hands-on experience and understanding in system calls, kernel programming, concurrency, synchronization, and elevator scheduling algorithms. Divided into three parts, it progressively builds skills.]

## Group Members
- **[Jada Doby]**: [jdd20a@fsu.edu]
- **[Shelley Bercy]**: [sb22bg@fsu.edu]
- **[Rood Vilmot]**: [rkv20@fsu.edu]
## Division of Labor

### Part 1: [System Call Tracing]
- **Responsibilities**: [Part 1 involves system-call tracing using strace, with participants creating an empty C program and its duplicate, adding five system calls to the latter, and verifying the difference in system calls produced. Emphasis is on minimizing additional function calls in the modified program.]
- **Assigned to**: [Jada Doby, Rood Vilmont]

### Part 2: [Timer Kernel Module]
- **Responsibilities**: [Create a Unix kernel module, "my_timer," utilizing ktime_get_real_ts64() to capture and store the current time. The module, when loaded, establishes a "/proc/timer" entry for displaying both the current time and elapsed time since the last call.]
- **Assigned to**: [Shelley Bercy, Rood Vilmont]
### Part 3a: [Adding System Calls]
- **Responsibilities**: [Prepare kernel for compilation, modify kernel files and define system calls]
- **Assigned to**: [Jada Doby, Shelley Bercy]
  ### Part 3b: [Kernel Compilation]
- **Responsibilities**: [ Compile the kernel with the system calls and check if it installed]
- **Assigned to**: [Rood Vilmont, Shelley Bercy]
  ### Part 3c: [Threads]
- **Responsibilities**: [ Utilize threads to control the elevator movement]
- **Assigned to**: [Jada Doby, Rood Vilmont]
  ### Part 3d: [Linked List]
- **Responsibilities**: [ Utilize a linked list to handle the passengers per floor/elevator]
- **Assigned to**: [Jada Doby, Shelley Bercy]
   ### Part 3e: [Mutexes]
- **Responsibilities**: [ Utilize a mutez to control the shared data access between floors and elevators]
- **Assigned to**: [ Shelley Bercy, Rood Vilmont, Jada Doby]
   ### Part 3f: [Scheduling Algorithm]
- **Responsibilities**: [ Develop the algorithm and use kmalloc to allocate the dynamic memory for the passengers]
- **Assigned to**: [Shelley Bercy, Rood Vilmont, Jada Doby ]
  


  

## File Listing
elevator/
├── Makefile
├── part1/
│   ├── empty.c
│   ├── empty.trace
│   ├── part1.c
│   ├── part1.trace
│   └── Makefile
├── part2/
│   ├── src/
│   │   ├── my_timer.c
│   │   ├── my_timer.h
│   │   └── Makefile
│   └── Makefile
├── part3/
│   ├── src/
│   │   ├── elevator.c
│   │   └── Makefile
│   ├── tests/
│   ├── Makefile
│   └── syscalls.c
├── Makefile
└── README.md



## How to Compile & Execute

### Requirements
- **Compiler**:`gcc` for C/C++
- **Dependencies**: <linux/kernel.h>, <linux/string.h>, <linux/proc_fs.h>, <linux/uaccess.h>, <linux/slab.h>, <linux/module.h>, <linux/init.h>, <linux/kthread.h> <linux/mutex.h>, <linux/list.h>, <linux/delay.h>,and <linux/linkage.h>.

### Compilation
For a C/C++ example:
```bash
make
```
This will build the executable in ...
### Execution
```To insert a kernel module :
     sudo insmod my_timer.ko
```
```To remove a kernel module :
     sudo rmmod my_timer.ko
```
```To check for our kernel module :
     lsmod|grep my_timer
```

## Bugs
- **Bug 1**: This is bug 1.
- **Bug 2**: This is bug 2.
- **Bug 3**: This is bug 3.

## Considerations
[Description]
