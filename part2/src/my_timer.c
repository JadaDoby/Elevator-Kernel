#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/timekeeping.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("cop4610");
MODULE_DESCRIPTION("A simple Linux driver for timing");
MODULE_VERSION("0.1");

static struct proc_dir_entry* my_timer_proc_entry;
static struct timespec64 last_time;
static bool has_previous_time = false;

static ssize_t my_timer_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos) {
    char buffer[256];
    int len;
    struct timespec64 now, elapsed;

    // Get current time
    ktime_get_real_ts64(&now);

    // Calculate elapsed time since last read
    if (has_previous_time) {
        elapsed = timespec64_sub(now, last_time);
        len = snprintf(buffer, sizeof(buffer), "Current Time: %lld.%09lu\nElapsed Time: %lld.%09lu\n",
                       (long long)now.tv_sec, now.tv_nsec,
                       (long long)elapsed.tv_sec, elapsed.tv_nsec);
    } else {
            len = snprintf(buffer, sizeof(buffer), "Current Time: %lld.%09lu\n",
                       (long long)now.tv_sec, now.tv_nsec);
            has_previous_time = true;
    }

    // Copy to user
    if (*ppos > 0 || count < len)
        return 0;
    if (copy_to_user(ubuf, buffer, len))
        return -EFAULT;

    *ppos = len;
    last_time = now; // Update last read time

    return len;
}

static const struct proc_ops my_timer_fops = {
    .proc_read = my_timer_read,
};

static int __init my_timer_init(void) {
    // Create proc entry
    my_timer_proc_entry = proc_create("my_timer", 0666, NULL, &my_timer_fops);
    if (!my_timer_proc_entry) {
        return -ENOMEM;
    }


    return 0;
}

static void __exit my_timer_exit(void) {
    // Remove proc entry
    proc_remove(my_timer_proc_entry);
  
}

module_init(my_timer_init);
module_exit(my_timer_exit);
