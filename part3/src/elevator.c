#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/sched.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Cop4610");
MODULE_DESCRIPTION("kernel module for pt3/elevator");

#define ENTRY_NAME "elevator"
#define MAX_PASSENGERS 5
#define MAX_WEIGHT 7

//global
int wait;  //waiting passengers
int helped; //helped passengers 

enum PassengerType {
    PART_TIME,
    LAWYER,
    BOSS,
    VISITOR,
};
static const int passenger_weights[4] = {
    10, // PART_TIME, equivalent to 1.0 weight
    15, // LAWYER, equivalent to 1.5 weight
    20, // BOSS, equivalent to 2.0 weight
    5   // VISITOR, equivalent to 0.5 weight
};


enum ElevatorState{
    OFFLINE,
    IDLE,
    LOADING,
    UP,
    DOWN
};

struct Passenger {
    int id;
    enum PassengerType type;
    int weight;
    int start_floor;
    int destination_floor;
    struct list_head struct_lister;
};

struct Floor {
    int floor_number;
    struct list_head passengers; // people waiting on floor
    struct mutex lock;   // mutex for floor
};

struct Elevator {
    struct list_head passengers; // people on the elevator 
    int passenger_count;
    int current_floor;
    int current_weight;
    int running;
    int stopped;
    enum ElevatorState status;  // the status of it 
    struct mutex lock; // mutex for elev
};


static struct Floor floors[5];  
static struct task_struct *elevator_thread;  //handles this thread

// Prototypes
static enum ElevatorState elevator_state=OFFLINE;
static int passengers_serviced=0;  //passenger serviced 

// The unload/loading functions 
static void load_passenger(struct Passenger *passenger, struct Elevator *elevator); // Added elevator parameter.
static void unload_passenger(struct Passenger *passenger); 

// Important structs 
static struct Elevator elevator;
static struct passenger passenger;

static struct proc_dir_entry *elevator_entry;  // start 


int start_elevator(void);
int issue_request(int start_floor, int destination_floor, int type);
int stop_elevator(void);
int movement(int cfloor, int dfloor );

extern int (*STUB_start_elevator)(void);
extern int (*STUB_issue_request)(int,int,int);
extern int (*STUB_stop_elevator)(void);


int process_passenger(struct Passenger *passenger) {   // gets passenger type
    int retval = 0;

    switch (passenger->type) {
        case PART_TIME:
            printk(KERN_INFO "Processing a PART_TIME passenger\n");
            return 'PART_TIME';

        case LAWYER:
            printk(KERN_INFO "Processing a LAWYER passenger\n");
             return 'LAWYER';

        case BOSS:
            printk(KERN_INFO "Processing a BOSS passenger\n");
            return 'BOSS';

        case VISITOR:
            printk(KERN_INFO "Processing a VISITOR passenger\n");
            return 'Vistor'

        default:
            retval = -EINVAL; 
             return'?'
    }

    return retval;
}


//start it 
int start_elevator(void) {
    mutex_lock(&elevator.lock);

    if (elevator.running == 1) {
        mutex_unlock(&elevator.lock);
        return 1;  
    } else {
        elevator.running = 1;
        elevator.stopped = 0;
        elevator.status = IDLE;
    }

    mutex_unlock(&elevator.lock);
    return 0; 
}



int issue_request(int start_floor, int destination_floor, int type) {
    if (type < PART_TIME || type > VISITOR || start_floor < 1 || start_floor > 5 ||
        destination_floor < 1 || destination_floor > 5) {
        return -EINVAL;
    }

    int weight = passenger_weights[type]; // Correctly use the lookup array

    mutex_lock(&floors[start_floor - 1].lock); // Correct floor indexing
    struct Passenger *new_passenger = kmalloc(sizeof(struct Passenger), GFP_KERNEL);
    if (!new_passenger) {
        mutex_unlock(&floors[start_floor - 1].lock);
        return -ENOMEM;
    }

    new_passenger->type = type;
    new_passenger->weight = weight; // Set weight correctly
    new_passenger->start_floor = start_floor;
    new_passenger->destination_floor = destination_floor;
    INIT_LIST_HEAD(&new_passenger->struct_lister);
    list_add_tail(&new_passenger->struct_lister, &floors[start_floor - 1].passengers); // Add to the correct floor
    mutex_unlock(&floors[start_floor - 1].lock);

    return 0;
}


bool check_for_waiting_passengers(void) {
    int i;
    // Assuming floors array starts from index 0 representing floor 1, up to index 4 representing floor 5
    for (i = 0; i < MAX_FLOOR; i++) {
        if (!list_empty(&floors[i].passengers)) { // Check if there are passengers waiting on floor i
            return true;
        }
    }
    return false; // No passengers waiting on any floor
}



static int elevator_thread(void *data) {
    while (!kthread_should_stop()) { // Keep running until the module is unloaded
        if (elevator.status != OFFLINE) { // Check if the elevator is operational
            // Movement towards the destination floor if not IDLE
            if (elevator.status == UP || elevator.status == DOWN) {
                elevator.current_floor = movement(elevator.current_floor, elevator.target_floor);

                if (elevator.current_floor == elevator.target_floor) {
                    // Arrived at the target, switch to LOADING state to unload/load passengers
                    elevator.status = LOADING;
                }
            }

            // Loading or unloading passengers if on the target floor
            if (elevator.status == LOADING) {
                unloading(); // Unload passengers first
                loading(); // Load new passenger
                msleep(1000); // Sleep to simulate time passing, adjust as needed

                // Determine next action: stay IDLE, or set a new target floor
                if (!list_empty(&elevator.passengers)) {
                    // Set target to the destination of the first passenger in the list
                    struct Passenger *first_passenger = list_first_entry(&elevator.passengers, struct Passenger, struct_lister);
                    elevator.target_floor = first_passenger->destination_floor;

                    // Decide movement direction based on the new target
                    if (elevator.current_floor < elevator.target_floor) {
                        elevator.status = UP;
                    } else if (elevator.current_floor > elevator.target_floor) {
                        elevator.status = DOWN;
                    }
                } else {
                    // No passengers left to service, remain IDLE or check for passengers waiting
                    elevator.status = check_for_waiting_passengers() ? LOADING : IDLE;
                }
            }
        }

        msleep(2000); // Sleep to simulate time passing, adjust as needed
    }
    return 0;
}


int stop_elevator(void) {
    printk(KERN_NOTICE "%s", __FUNCTION__);
    if (mutex_lock_interruptible(&elevator.lock) == 0) {
        if (elevator.running == 0 && elevator.stopped == 1) {
            mutex_unlock(&elevator.lock);
            return 1; 
        }
        elevator.stopped = 1;
        elevator.running = 0;
        mutex_unlock(&elevator.lock);
    }
    return 0;
}


int movement(int cfloor, int dfloor)
{
switch(cfloor-dfloor){
    case 0:
      elevator.status= IDLE;
      return dfloor;
    case 1 ... INT_MAX:
        elevator.status=UP;
        break;
    case INT_MIN... -1:
        elevator.status=DOWN;
        break;
}
    msleep(2);
    return (cfloor +(elevator.status==UP ? 1:-1)); // like a if-else statement
}





int loading(void) {
    struct list_head *temp, *dummy;
    struct Passenger *passenger;

    if (mutex_lock_interruptible(&elevator.lock) == 0) {
        list_for_each_safe(temp, dummy, &floors[elevator.current_floor].passengers) {
            passenger = list_entry(temp, struct Passenger, struct_lister);
            int passenger_weight = passenger_weights[passenger->type]; // Use the lookup array

            // Check if the elevator can accommodate the passenger
            if (elevator.current_weight + passenger_weight <= MAX_WEIGHT * 10 && 
                elevator.passenger_count < MAX_PASSENGERS) {
                list_move_tail(temp, &elevator.passengers);
                elevator.current_weight += passenger_weight;
                elevator.passenger_count++;
            } else {
                // Elevator full or would exceed weight limit
                break;
            }
        }
        mutex_unlock(&elevator.lock);
    }
    return 0;
}

int unloading(void) {
    struct list_head *temp, *dummy;
    struct passenger *p;

    if (mutex_lock_interruptible(&elevator.lock) == 0) {
        list_for_each_safe(temp, dummy, &elevator.passengers) {
            p = list_entry(temp, struct passenger, list);

            if (p->destination_floor == elevator.current_floor) {
                // Remove passenger from elevator
                list_del(temp);
                kfree(p); // Free the memory allocated for the passenger
                elevator.current_weight -= get_passenger_weight(p->type); // Adjust weight
                elevator.current_passengers--;
                passengers_serviced++; // increment serviced counter
            }
        }
        mutex_unlock(&elevator.lock);
    }
    return 0;
}

// Proc Information
//generates a string describing elevator state for user-space consumption.
static int __init elevator_init(void) {
    int i;

    // Initialize elevator
    elevator.current_floor = 1; // Assuming ground floor is 1
    elevator.current_weight = 0;
    elevator.passenger_count = 0;
    elevator.status = OFFLINE;
    mutex_init(&elevator.lock);

    // Initialize floors
    for (i = 0; i < 5; i++) {
        mutex_init(&floors[i].lock);
        INIT_LIST_HEAD(&floors[i].passengers);
    }

    printk(KERN_INFO "Elevator module loaded\n");
    return 0;
}





static ssize_t elevator_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos) {
    char buf[10000];
    int len = 0;
    struct Passenger *pass;
    int i;

    // Elevator state
    const char *status_str[] = { "OFFLINE", "IDLE", "LOADING", "UP", "DOWN" };
    len += sprintf(buf + len, "Elevator state: %s\n", status_str[elevator.status]);

    // Current floor and load
    len += sprintf(buf + len, "Current floor: %d\n", elevator.current_floor);
    len += sprintf(buf + len, "Current load: %d lbs\n", elevator.current_weight);

    // List of passengers in the elevator
    len += sprintf(buf + len, "Elevator status: ");
    list_for_each_entry(pass, &elevator.passengers, struct_lister) {
        char typeChar = passenger_type_to_char(pass->type); // Implement this function based on your logic
        len += sprintf(buf + len, "%c%d ", typeChar, pass->destination_floor);
    }
    len += sprintf(buf + len, "\n");

    // Number of passengers on each floor
    for (i = 0; i < 5; i++) {
        len += sprintf(buf + len, "[%c] Floor %d: ", (i == elevator.current_floor ? '*' : ' '), i + 1);
        struct Passenger *floor_pass;
        list_for_each_entry(floor_pass, &floors[i].passengers, struct_lister) {
            char typeChar = passenger_type_to_char(floor_pass->type); // Again, implement based on your logic
            len += sprintf(buf + len, "%c%d ", typeChar, floor_pass->destination_floor);
        }
        len += sprintf(buf + len, "\n");
    }

    // Number of passengers, passengers waiting, and passengers serviced
    len += sprintf(buf + len, "Number of passengers: %d\n", elevator.passenger_count);
    len += sprintf(buf + len, "Number of passengers waiting: %d\n", wait);
    len += sprintf(buf + len, "Number of passengers serviced: %d\n", helped);

    // Use simple_read_from_buffer
    return simple_read_from_buffer(ubuf, count, ppos, buf, len);
}


static int __init elevator_init(void) {
    int i;

    // Initialize elevator
    elevator.current_floor = 1; // Assuming ground floor is 1
    elevator.current_weight = 0;
    elevator.passenger_count = 0;
    elevator.status = OFFLINE;
    mutex_init(&elevator.lock);

    // Initialize floors
    for (i = 0; i < 5; i++) {
        mutex_init(&floors[i].lock);
        INIT_LIST_HEAD(&floors[i].passengers);
    }

     elevator_entry = proc_create(ENTRY_NAME, 0666, NULL, &elevator_fops);
    if (!elevator_entry) {
        printk(KERN_ALERT "Error creating proc entry\n");
        return -ENOMEM;


    printk(KERN_INFO "Elevator module loaded\n");
    return 0;
}

}


static const struct proc_ops elevator_fops = {
    .proc_read = elevator_read,
};


static void __exit elevator_exit(void) {
    struct list_head *temp, *dummy;
    struct Passenger *passenger;
    int i;

    // Stop the elevator thread
    if (elevator_thread) {
        kthread_stop(elevator_thread);
    }

    // Iterate over each floor to free waiting passengers
    for (i = 0; i < 5; i++) {
        mutex_lock(&floors[i].lock);
        list_for_each_safe(temp, dummy, &floors[i].passengers) {
            passenger = list_entry(temp, struct Passenger, struct_lister);
            list_del(temp);
            kfree(passenger);
        }
        mutex_unlock(&floors[i].lock);
    }

    // Clear any passengers currently in the elevator
    mutex_lock(&elevator.lock);
    list_for_each_safe(temp, dummy, &elevator.passengers) {
        passenger = list_entry(temp, struct Passenger, struct_lister);
        list_del(temp);
        kfree(passenger);
    }
    mutex_unlock(&elevator.lock);

    // Free other resources if necessary (e.g., destroy mutexes)
    mutex_destroy(&elevator.lock);
    for (i = 0; i < 5; i++) {
        mutex_destroy(&floors[i].lock);
    }

    // Remove the proc entry if it exists
    if (elevator_entry) {
        proc_remove(elevator_entry);
    }

    printk(KERN_INFO "Elevator module exited\n");
}

module_init(elevator_init);
module_exit(elevator_exit);

