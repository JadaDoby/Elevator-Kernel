#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/mutex.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Office Elevator Scheduling Kernel Module");
MODULE_VERSION("0.1");

#define MAX_WEIGHT 7
#define MAX_PASSENGERS 5
#define FLOORS 5

typedef enum {
    PART_TIME,
    LAWYER,
    BOSS,
    VISITOR
} PassengerType;

static const float weights[] = {1.0, 1.5, 2.0, 0.5};

typedef struct Passenger {
    int start_floor;
    int destination_floor;
    PassengerType type;
    struct Passenger *next;
} Passenger;

typedef struct {
    Passenger *head;
    int num_passengers;
    float total_weight;
} Elevator;

static Elevator elevator;
static DEFINE_MUTEX(elevator_lock);
static bool elevator_active = false;

// Function prototypes
static int start_elevator(void);
static int issue_request(int start_floor, int destination_floor, int type);
static int stop_elevator(void);

static int __init elevator_init(void) {
    printk(KERN_INFO "Elevator module is being loaded.\n");
    elevator.head = NULL;
    elevator.num_passengers = 0;
    elevator.total_weight = 0;
    elevator_active = true;
    return 0;
}

static void __exit elevator_exit(void) {
    Passenger *tmp;
    while (elevator.head != NULL) {
        tmp = elevator.head;
        elevator.head = elevator.head->next;
        kfree(tmp);
    }
    printk(KERN_INFO "Elevator module is being unloaded.\n");
}

static int start_elevator(void) {
    if (elevator_active) {
        return 1; // Elevator is already active
    }
    mutex_lock(&elevator_lock);
    elevator_active = true;
    mutex_unlock(&elevator_lock);
    printk(KERN_INFO "Elevator started.\n");
    return 0;
}

static int issue_request(int start_floor, int destination_floor, int type) {
    Passenger *new_passenger;
    float weight;

    if (!elevator_active || start_floor < 1 || start_floor > FLOORS || 
        destination_floor < 1 || destination_floor > FLOORS || type < PART_TIME || type > VISITOR) {
        return 1; // Invalid request
    }

    weight = weights[type];
    mutex_lock(&elevator_lock);
    if ((elevator.total_weight + weight) > MAX_WEIGHT || elevator.num_passengers >= MAX_PASSENGERS) {
        mutex_unlock(&elevator_lock);
        return -1; // Cannot add passenger due to weight/capacity limit
    }

    new_passenger = kmalloc(sizeof(Passenger), GFP_KERNEL);
    if (!new_passenger) {
        mutex_unlock(&elevator_lock);
        return -ENOMEM; // Memory allocation failed
    }

    new_passenger->start_floor = start_floor;
    new_passenger->destination_floor = destination_floor;
    new_passenger->type = type;
    new_passenger->next = elevator.head;
    elevator.head = new_passenger;
    elevator.num_passengers++;
    elevator.total_weight += weight;
    mutex_unlock(&elevator_lock);

    printk(KERN_INFO "Passenger request issued successfully.\n");
    return 0;
}

static int stop_elevator(void) {
    if (!elevator_active) {
        return 1; // Elevator is already stopped or in the process of stopping
    }

    mutex_lock(&elevator_lock);
    elevator_active = false;
    mutex_unlock(&elevator_lock);
    printk(KERN_INFO "Elevator stopped.\n");

    // Logic to offload all passengers should be implemented here
    return 0;
}

module_init(elevator_init);
module_exit(elevator_exit);
