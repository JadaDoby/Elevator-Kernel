# Group18_P2
# [Elevator]

[This project is designed to provide hands-on experience and understanding in system calls, kernel programming, concurrency, synchronization, and elevator scheduling algorithms. Divided into three parts, it progressively builds skills.

**Part 1: System Calls Integration**
- Introduces system calls and their integration into a C program.
- Uses "strace" tool for verification, laying the foundation for kernel interaction.

**Part 2: Kernel Programming**
- Develops a kernel module, "my_timer," utilizing timekeeping functions.
- Creates a proc entry for reading current time, deepening knowledge of kernel modules and proc interfaces.

**Part 3: Elevator Scheduling**
- Implements a scheduling algorithm for a dorm elevator in a kernel module.
- Handles concurrency, synchronization, and efficient scheduling.
- Challenges participants with a practical, advanced kernel programming task.

This project's structure ensures a seamless progression, with each part building on the knowledge gained in the previous sections. Participants gain practical skills in system calls, kernel programming, and key concepts crucial for developing robust software systems, particularly in operating systems and low-level programming domains.]

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
### Part 3: [Elevator Module]
- **Responsibilities**: [In this task, you are instructed to implement a scheduling algorithm for an office elevator using a kernel module named "elevator." The elevator can hold up to 5 passengers with a maximum weight of 7 lbs, and each worker is randomly assigned as a part-time worker, lawyer, boss, or visitor, each with specific weights. The task involves adding three system calls (start_elevator, issue_request, stop_elevator) to control the elevator and create passengers, and then compiling and installing the modified kernel. Finally, testing the added system calls is performed using provided test scripts to ensure successful implementation.]
- **Assigned to**: [Jada Doby,Shelley Bercy, Rood Vilmont]

  

## File Listing
...
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
│   │   └── Makefile
│   │   ├── my_timer.h
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
- **Dependencies**:<linux/kernel.h>, <linux/string.h>, <linux/proc_fs.h>, <linux/uaccess.h>, <linux/slab.h>, <linux/module.h>, <linux/init.h>, <linux/kthread.h> <linux/mutex.h>, <linux/list.h>, <linux/delay.h>,and <linux/linkage.h>.

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
