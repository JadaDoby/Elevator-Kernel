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
#define MAX_WEIGHT 70
#define MAX_FLOOR 5 // Added for floor indexing
#define MIN_FLOOR 1 // Added for floor indexing
#define PROC_BUF_SIZE 10000

// Global variables
int wait;   // waiting passengers
int helped; // helped passengers

enum PassengerType
{
    PART_TIME,
    LAWYER,
    BOSS,
    VISITOR,
};
const char *passType[] = {"PART_TIME", "LAWYER", "BOSS", "VISITOR"};
static const int passenger_weights[4] = {10, 15, 20, 5}; 

enum ElevatorState
{
    OFFLINE,
    IDLE,
    LOADING,
    UP,
    DOWN
};
const char *status_str[] = {"OFFLINE", "IDLE", "LOADING", "UP", "DOWN"};
struct Passenger
{
    enum PassengerType type;
    int weight;
    int start_floor;
    int destination_floor;
    struct list_head struct_lister;
};

struct Floor
{
    int floor_number;
    struct list_head passengers; // people waiting on floor
    struct mutex lock;           // mutex for floor
};

struct Elevator
{
    struct list_head passengers; // people on the elevator
    int passenger_count;
    int current_floor;
    int current_weight;
    int running;
    int stopped;
    enum ElevatorState status; // the status of it
    int target_floor;          // the destination floor
    struct mutex lock;         // mutex for elevator
};

static struct Floor floors[5];
static struct task_struct *elevator_thread; // handles this thread

extern int (*STUB_issue_request)(int, int, int);
extern int (*STUB_start_elevator)(void);
extern int (*STUB_stop_elevator)(void);

// Prototypes
static int start_elevator(void);
static int issue_request(int start_floor, int destination_floor, int type);
static int stop_elevator(void);
static int movement(int cfloor, int dfloor);
static int process_passenger(struct Passenger *passenger);
static int loading(void);   
static int unloading(void); 

// Important structs
static struct Elevator elevator;
static struct Passenger passenger;
static struct proc_dir_entry *elevator_entry; // start
static const struct proc_ops elevator_fops;

int start_elevator(void)
{

    mutex_lock(&elevator.lock);

    if (elevator.running == 1)
    {
        mutex_unlock(&elevator.lock);
        return 1;
    }
    else
    {
        elevator.status = IDLE;
        elevator.current_floor = 1;
        elevator.current_weight = 0;
        elevator.passenger_count = 0;
        elevator.running = 1;
        elevator.stopped = 0; 
    }

    mutex_unlock(&elevator.lock);
    return 0;
}

int issue_request(int start_floor, int destination_floor, int type)
{
   

    if (type < PART_TIME || type > VISITOR || start_floor < 1 || start_floor > 5 || destination_floor < 1 ||
        destination_floor > 5)
    {
        return -EINVAL;
    }

    int weight = passenger_weights[type]; 
  

    mutex_lock(&floors[start_floor - 1].lock); // Correct floor indexing

    struct Passenger *new_passenger = kmalloc(sizeof(struct Passenger), GFP_KERNEL);
    if (!new_passenger)
    {
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

bool check_for_waiting_passengers(void)
{
    for (int i = 0; i < MAX_FLOOR; i++)
    {
        if (!list_empty(&floors[i].passengers))
        {
            return true;
        }
    }
    return false; // No passengers waiting on any floor
}

static int movement(int cfloor, int dfloor)
{
   
    int new_floor;
    if (cfloor == dfloor)
    {
        elevator.status = IDLE;  
        return dfloor;
    }
    else if (cfloor < dfloor)
    {
        elevator.status = UP;
        new_floor = cfloor + 1;
    }
    else
    {
        elevator.status = DOWN;
        new_floor = cfloor - 1;
    }
    msleep(2000); // Simulate movement delay
    return new_floor; // Return the new floor after movement
}

static int loading(void)
{
    struct list_head *temp, *dummy;
    struct Passenger *passenger;
    int passenger_weight;

    if (mutex_lock_interruptible(&elevator.lock) == 0)
    {
        list_for_each_safe(temp, dummy, &floors[elevator.current_floor - 1].passengers)
        {
            passenger = list_entry(temp, struct Passenger, struct_lister);
            passenger_weight = passenger_weights[passenger->type]; // Use the lookup array

            // Check if the elevator can accommodate the passenger
            if (elevator.current_weight + passenger_weight <= MAX_WEIGHT && elevator.passenger_count < MAX_PASSENGERS)
            {
                list_move_tail(temp, &elevator.passengers);
                elevator.current_weight += passenger_weight;
                elevator.passenger_count++;
            }
            else
            {
                // Elevator full or would exceed weight limit
                break;
            }
        }
        mutex_unlock(&elevator.lock);
    }
    return 0;
}

static int unloading(void)
{
    struct list_head *temp, *dummy;
    struct Passenger *p;

    if (mutex_lock_interruptible(&elevator.lock) == 0)
    {
        list_for_each_safe(temp, dummy, &elevator.passengers)
        {
            p = list_entry(temp, struct Passenger, struct_lister);

            if (p->destination_floor == elevator.current_floor)
            {
                // Remove passenger from elevator
                list_del(temp);
                kfree(p);                                              // Free the memory allocated for the passenger
                elevator.current_weight -= passenger_weights[p->type]; // Adjust weight
                elevator.passenger_count--;
                helped++; // Increment serviced counter
            }
        }
        mutex_unlock(&elevator.lock);
    }
    return 0;
}

int elevator_thread_fn(void *data)
{
   
    while (!kthread_should_stop())
    { 
        if (elevator.running == 1 && elevator.status != OFFLINE)
        {
          
            // Simulate elevator moving up and down between floors
            for (elevator.current_floor = MIN_FLOOR; elevator.current_floor <= MAX_FLOOR; elevator.current_floor++)
            {
                mutex_lock(&elevator.lock);
                elevator.status = LOADING; // Set the status to LOADING when at a floor
                mutex_unlock(&elevator.lock);
                loading();   // Load passengers at the current floor
                unloading(); // Unload passengers at the current floor
                msleep(1000); // Sleep to simulate the time taken to move between floors

                // Update the status to moving (UP or DOWN) after loading/unloading
                if (elevator.current_floor < MAX_FLOOR)
                {
                    mutex_lock(&elevator.lock);
                    elevator.status = UP;
                    mutex_unlock(&elevator.lock);
                }
                msleep(2000); // Sleep to simulate the time taken to move between floors
            }

            // Once it reaches the top, it goes back down
            for (elevator.current_floor = MAX_FLOOR; elevator.current_floor >= MIN_FLOOR; elevator.current_floor--)
            {
                mutex_lock(&elevator.lock);
                elevator.status = LOADING; // Set the status to LOADING when at a floor
                mutex_unlock(&elevator.lock);

                loading();   // Load passengers at the current floor
                unloading(); // Unload passengers at the current floor
                msleep(1000); // Sleep to simulate the time taken to move between floors

                if (elevator.current_floor > MIN_FLOOR)
                {
                    mutex_lock(&elevator.lock);
                    elevator.status = DOWN;
                    mutex_unlock(&elevator.lock);
                }

                msleep(2000); // Sleep to simulate the time taken to move between floors
            }
        }
        else if (elevator.stopped == 1 && list_empty(&elevator.passengers) && !check_for_waiting_passengers())
        {
            // If the elevator is stopping, and there are no passengers left, it goes offline
            mutex_lock(&elevator.lock);
            elevator.running = 0;
            elevator.status = IDLE;
            mutex_unlock(&elevator.lock);
        }

        // Check if there are any passengers waiting or on the elevator before deciding on further action
        if (list_empty(&elevator.passengers) && !check_for_waiting_passengers())
        {
            // If there are no passengers waiting or on the elevator, set the status to IDLE
            mutex_lock(&elevator.lock);
            elevator.status = IDLE;
            mutex_unlock(&elevator.lock);
        }
    }

    return 0;
}

int stop_elevator(void)
{
    // Acquire the elevator lock to safely check and modify its state.
    if (mutex_lock_interruptible(&elevator.lock) == 0)
    {

        // Check if the elevator is already in the process of deactivating or is already stopped.
        if (elevator.running == 0)
        {
            mutex_unlock(&elevator.lock);
            return 1; // Elevator is already stopped or being stopped.
        }

        // Initiate the stopping process by setting the elevator's state accordingly.
        elevator.stopped = 1; // Indicate that the elevator is in the process of stopping.

        // Instead of stopping immediately, let the elevator_thread function handle the gradual stopping,
        // ensuring all passengers are offloaded before complete deactivation.
        if (list_empty(&elevator.passengers))
        {
            // If there are no passengers in the elevator, we can deactivate it immediately.
            elevator.running = 0;
            elevator.status = OFFLINE; // Set elevator state to OFFLINE.
        }
        else
        {
            // There are still passengers in the elevator. Keep the elevator in the stopping process,
            // allowing the running elevator_thread to offload them before complete deactivation.
        }

        mutex_unlock(&elevator.lock);
    }

    // Since the actual stopping might be deferred until passengers are offloaded,
    // we return 0 to indicate the stop process has begun but not necessarily completed.
    return 0;
}

static ssize_t elevator_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos)
{
    char buf[PROC_BUF_SIZE]; // Define buffer size for our output
    int len = 0;
    ssize_t ret;
    struct Passenger *pass;


    // Elevator state descriptions
    len += snprintf(buf + len, PROC_BUF_SIZE - len, "Elevator state: %s\n", status_str[elevator.status]);

    // Current floor and load
    len += snprintf(buf + len, PROC_BUF_SIZE - len, "Current floor: %d\n", elevator.current_floor);
    len += snprintf(buf + len, PROC_BUF_SIZE - len, "Current load: %d lbs\n", elevator.current_weight);

    // List of passengers in the elevator
    len += snprintf(buf + len, PROC_BUF_SIZE - len, "Elevator passengers: ");
    list_for_each_entry(pass, &elevator.passengers, struct_lister)
    {
        char typeChar = 'A' + pass->type; // Assuming enum PassengerType starts at 0
        len += snprintf(buf + len, PROC_BUF_SIZE - len, "%c%d ", typeChar, pass->destination_floor);
    }
    len += snprintf(buf + len, PROC_BUF_SIZE - len, "\n");

    // Number of passengers on each floor
    for (int i = 0; i < MAX_FLOOR; i++)
    {
        len += snprintf(buf + len, PROC_BUF_SIZE - len, "[%c] Floor %d: ", (i + 1 == elevator.current_floor) ? '*' : ' ', i + 1);
        list_for_each_entry(pass, &floors[i].passengers, struct_lister)
        {
            char typeChar = 'A' + pass->type; // Assuming enum PassengerType starts at 0
            len += snprintf(buf + len, PROC_BUF_SIZE - len, "%c%d ", typeChar, pass->destination_floor);
        }
        len += snprintf(buf + len, PROC_BUF_SIZE - len, "\n");
    }

    // Number of passengers, passengers waiting, and passengers serviced
    len += snprintf(buf + len, PROC_BUF_SIZE - len, "Number of passengers: %d\n", elevator.passenger_count);
    len += snprintf(buf + len, PROC_BUF_SIZE - len, "Number of passengers waiting: %d\n", wait);
    len += snprintf(buf + len, PROC_BUF_SIZE - len, "Number of passengers serviced: %d\n", helped);


    ret = simple_read_from_buffer(ubuf, count, ppos, buf, len);

    return ret;
}

static const struct proc_ops elevator_fops = {
    .proc_read = elevator_read,
};

static int __init elevator_init(void)
{

    STUB_start_elevator = start_elevator;
    STUB_issue_request = issue_request;
    STUB_stop_elevator = stop_elevator;
    // Initialize elevator
    elevator.current_floor = 1; // Assuming ground floor is 1
    elevator.current_weight = 0;
    elevator.passenger_count = 0;
    elevator.status = OFFLINE;
    elevator.target_floor = 2; // Set initial target floor
    mutex_init(&elevator.lock);
    INIT_LIST_HEAD(&elevator.passengers);

    // Initialize floors
    for (int i = 0; i < 5; i++)
    {
        mutex_init(&floors[i].lock);
        INIT_LIST_HEAD(&floors[i].passengers);
        // this shows the passengers on the floor
    }

    // Create proc entry
    elevator_entry = proc_create(ENTRY_NAME, 0666, NULL, &elevator_fops);
    if (!elevator_entry)
    {
        return -ENOMEM;
    }

    // Start elevator thread
    elevator_thread = kthread_run(elevator_thread_fn, NULL, "elevator_thread");
    if (IS_ERR(elevator_thread))
    {
        return PTR_ERR(elevator_thread);
    }
    return 0;
}

static void __exit elevator_exit(void)
{
    struct list_head *temp, *dummy;
    struct Passenger *passenger;

    // Stop the elevator thread
    if (elevator_thread)
    {
        kthread_stop(elevator_thread);
    }

    // Iterate over each floor to free waiting passengers
    for (int i = 0; i < 5; i++)
    {
        mutex_lock(&floors[i].lock);
        list_for_each_safe(temp, dummy, &floors[i].passengers)
        {
            passenger = list_entry(temp, struct Passenger, struct_lister);
            list_del(temp);
            kfree(passenger);
        }
        mutex_unlock(&floors[i].lock);
    }

    // Clear any passengers currently in the elevator
    mutex_lock(&elevator.lock);
    list_for_each_safe(temp, dummy, &elevator.passengers)
    {
        passenger = list_entry(temp, struct Passenger, struct_lister);
        list_del(temp);
        kfree(passenger);
    }
    mutex_unlock(&elevator.lock);

    // Free other resources if necessary (e.g., destroy mutexes)
    mutex_destroy(&elevator.lock);
    for (int i = 0; i < 5; i++)
    {
        mutex_destroy(&floors[i].lock);
    }

    STUB_start_elevator = NULL;
    STUB_issue_request = NULL;
    STUB_stop_elevator = NULL;

    // Remove the proc entry if it exists
    if (elevator_entry)
    {
        proc_remove(elevator_entry);
    }

}

module_init(elevator_init);
module_exit(elevator_exit);
