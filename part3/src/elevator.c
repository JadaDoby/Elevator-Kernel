#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include <linux/mutex.h>
#include <linux/delay.h>

#define MAX_PASSENGERS 5
#define MAX_WEIGHT 7

enum PassengerType {
    PART_TIME,
    LAWYER,
    BOSS,
    VISITOR
};

struct Passenger {
    int id;
    enum PassengerType type;
    int weight;
    int start_floor;
    int destination_floor;
    struct list_head struct_lister;  // Linked list node for floors or elevator
};

struct Floor {
    int floor_number;
    struct list_head passengers;  // List of passengers waiting on this floor
    struct mutex lock;  // Mutex for controlling access to the passenger list
};

struct Elevator {
    struct list_head passengers;  // List of passengers inside the elevator
    int current_floor;
    struct mutex lock;  // Mutex for controlling access to the elevator state
};

static struct Elevator elevator;
static struct Floor floors[5];  // Assuming 5 floors in the building

static struct task_struct *elevator_thread;

DECLARE_WAIT_QUEUE_HEAD(elevator_wait_queue);

static void load_passenger(struct Passenger *passenger) {
    pr_info("Loading passenger %d (Type: %d) at floor %d\n", passenger->id, passenger->type, passenger->start_floor);
    msleep(1000); // Simulate loading time

    mutex_lock(&elevator.lock);
    list_add_tail(&passenger->struct_lister, &elevator.passengers);
    mutex_unlock(&elevator.lock);
}

static void unload_passenger(struct Passenger *passenger) {
    pr_info("Unloading passenger %d at floor %d\n", passenger->id, elevator.current_floor);
    msleep(1000); // Simulate unloading time

    mutex_lock(&elevator.lock);
    list_del(&passenger->struct_lister);
    mutex_unlock(&elevator.lock);

    kfree(passenger);
}

static int elevator_schedule(void *data) {
    while (!kthread_stop(elevator_thread)) {
        struct Passenger *passenger;
        int next_floor;

        // Check if there are passengers
        if (list_empty(&elevator.passengers)) {
            pr_info("Elevator is idle...\n");
            msleep(2000); // Wait for 2 seconds if idle
            continue;
        }

        // Move to the next floor
        next_floor = list_entry(elevator.passengers.next, struct Passenger, struct_lister)->start_floor;
        pr_info("Moving to floor %d...\n", next_floor);
        msleep(2000); // Simulate moving time

        // Unload passengers at the current floor
        mutex_lock(&elevator.lock);
        list_for_each_entry(passenger, &elevator.passengers, struct_lister) {
            if (passenger->destination_floor == elevator.current_floor) {
                unload_passenger(passenger);
                wake_up_interruptible(&elevator_wait_queue);
            }
        }
        mutex_unlock(&elevator.lock);

        // Load passengers at the current floor
        mutex_lock(&floors[elevator.current_floor - 1].lock);
        list_for_each_entry(passenger, &floors[elevator.current_floor - 1].passengers, struct_lister) {
            load_passenger(passenger);
            wake_up_interruptible(&elevator_wait_queue);
        }
        mutex_unlock(&floors[elevator.current_floor - 1].lock);

        // Move to the next floor
        elevator.current_floor = next_floor;
    }

    return 0;
}

static int add_passenger(enum PassengerType type, int start_floor, int dest_floor) {
    struct Passenger *passenger;

    passenger = kmalloc(sizeof(struct Passenger), GFP_KERNEL);
    if (!passenger)
        return -ENOMEM;

    passenger->id = prandom_u32();
    passenger->type = type;

    switch (type) {
        case PART_TIME:
            passenger->weight = 1;
            break;
        case LAWYER:
            passenger->weight = 1.5;
            break;
        case BOSS:
            passenger->weight = 2;
            break;
        case VISITOR:
            passenger->weight = 0.5;
            break;
        default:
            passenger->weight = 1;
            break;
    }

    passenger->start_floor = start_floor;
    passenger->destination_floor = dest_floor;

    INIT_LIST_HEAD(&passenger->struct_lister);

    if (start_floor == dest_floor) {
        // Passenger's destination is the same as the starting floor; unload immediately
        unload_passenger(passenger);
    } else {
        // Add passenger to the list of waiting passengers on the starting floor
        mutex_lock(&floors[start_floor - 1].lock);
        list_add_tail(&passenger->struct_lister, &floors[start_floor - 1].passengers);
        mutex_unlock(&floors[start_floor - 1].lock);
    }

    return 0;
}

static int elevator_init(void) {
    pr_info("Elevator module loaded\n");

    elevator.current_floor = 1;

    // Initialize floors and mutexes
    for (int i = 0; i < 5; ++i) {
        INIT_LIST_HEAD(&floors[i].passengers);
        floors[i].floor_number = i + 1;
        mutex_init(&floors[i].lock);
    }

    INIT_LIST_HEAD(&elevator.passengers);
    mutex_init(&elevator.lock);

    elevator_thread = kthread_run(elevator_schedule, NULL, "elevator_thread");
    if (IS_ERR(elevator_thread)) {
        pr_err("Failed to create elevator thread\n");
        return PTR_ERR(elevator_thread);
    }

    // Example: Add passengers
    add_passenger(PART_TIME, 2, 4);
    add_passenger(LAWYER, 1, 3);

    return 0;
}

static void elevator_exit(void) {
    kthread_stop(elevator_thread);

    // Unload passengers from elevator
    struct Passenger *passenger, *temp_passenger;
    mutex_lock(&elevator.lock);
    list_for_each_entry_safe(passenger, temp_passenger, &elevator.passengers, struct_lister) {
        unload_passenger(passenger);
    }
    mutex_unlock(&elevator.lock);

    // Unload passengers from floors
    for (int i = 0; i < 5; ++i) {
        mutex_lock(&floors[i].lock);
        list_for_each_entry_safe(passenger, temp_passenger, &floors[i].passengers, struct_lister) {
            list_del(&passenger->struct_lister);
            kfree(passenger);
        }
        mutex_unlock(&floors[i].lock);
    }

    pr_info("Elevator module unloaded\n");
}

//printk

module_init(elevator_init);
module_exit(elevator_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR(" Group 18");
MODULE_DESCRIPTION("Elevator Scheduling Module");
