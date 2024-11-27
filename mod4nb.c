/*
 * Exemple simple de test des fonctions timer, clock et autres
 * Le code dépend de la version du noyau cible !!
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/proc_fs.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/uaccess.h>
#include <linux/semaphore.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 0, 0)
#include <linux/timekeeping.h>
#endif

MODULE_AUTHOR("P. Foubet");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("Adaptation du module 4 pour les noyaux 3.x à 6.x");

int delay = HZ; /* le délai par défaut */

module_param(delay, int, S_IRUGO);

enum hor_files
{
    HOR_BUSY,
    HOR_SCHED,
    HOR_WAIT,
    HOR_SCHEDTO
};

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 3, 0)
static char buf[256];
static struct semaphore Sem; /* Sémaphore associé au buffer */
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 3, 0)
int hor_fct(char *buf, char **start, off_t offset, int len, int *eof, void *p)
#else
ssize_t hor_fct(struct file *fp, char __user *bufu, size_t len, loff_t *offp)
#endif
{
    unsigned long j0, j1; /* jiffies */
    wait_queue_head_t wait;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 0, 0)
    char *p = proc_get_parent_data(file_inode(fp)); // Pour les noyaux >= 5.0
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(3, 3, 0)
    char *p = PDE_DATA(file_inode(fp)); // Pour les noyaux entre 3.3 et 5.0
#endif

    init_waitqueue_head(&wait);
    j0 = jiffies;
    j1 = j0 + delay;

    switch ((long)p)
    {
    case HOR_BUSY:
        while (time_before(jiffies, j1))
            cpu_relax();
        break;
    case HOR_SCHED:
        while (time_before(jiffies, j1))
            schedule();
        break;
    case HOR_WAIT:
        wait_event_interruptible_timeout(wait, 0, delay);
        break;
    case HOR_SCHEDTO:
        set_current_state(TASK_INTERRUPTIBLE);
        schedule_timeout(delay);
        break;
    }
    j1 = jiffies; /* Valeur après attente */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 3, 0)
    if (down_interruptible(&Sem))
        return 0;
#endif
    len = sprintf(buf, "%9li %9li %9li\n", j0, j1, j1 - j0);

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 3, 0)
    *start = buf;
#else
    if (copy_to_user(bufu, buf, len))
        return -EFAULT;
    up(&Sem);
#endif
    return len;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 3, 0)
int hor_clock(char *buf, char **start, off_t offset, int len, int *eof, void *p)
#else
ssize_t hor_clock(struct file *fp, char __user *bufu, size_t len, loff_t *offp)
#endif
{
    unsigned long j1;
    u64 j2;

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 0, 0)
    struct timeval tv1;
    struct timespec tv2;

    j1 = jiffies;
    j2 = get_jiffies_64();
    do_gettimeofday(&tv1);
    tv2 = current_kernel_time();

    len = sprintf(buf, "0x%08lx 0x%016Lx %10i.%06i\n"
                       "%40i.%09i\n",
                  j1, j2,
                  (int)tv1.tv_sec, (int)tv1.tv_usec,
                  (int)tv2.tv_sec, (int)tv2.tv_nsec);
#else
    u64 time_ns = ktime_get_real_ns();

    j1 = jiffies;
    j2 = get_jiffies_64();

    len = sprintf(buf, "0x%08lx 0x%016Lx Time in nanoseconds: %llu\n",
                  j1, j2, time_ns);
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 3, 0)
    *start = buf;
#else
    if (copy_to_user(bufu, buf, len))
        return -EFAULT;
#endif
    return len;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
static const struct proc_ops fops_hor_clock = {
    .proc_read = hor_clock,
};

static const struct proc_ops fops_hor_fct = {
    .proc_read = hor_fct,
};
#else
static struct file_operations fops_hor_clock = {
    .owner = THIS_MODULE,
    .read = hor_clock,
};

static struct file_operations fops_hor_fct = {
    .owner = THIS_MODULE,
    .read = hor_fct,
};
#endif

int __init hor_init(void)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 3, 0)
    create_proc_read_entry("HORclock", 0, NULL, hor_clock, NULL);
    create_proc_read_entry("HORbusy", 0, NULL, hor_fct, (void *)HOR_BUSY);
    create_proc_read_entry("HORsched", 0, NULL, hor_fct, (void *)HOR_SCHED);
    create_proc_read_entry("HORwait", 0, NULL, hor_fct, (void *)HOR_WAIT);
    create_proc_read_entry("HORsched2", 0, NULL, hor_fct, (void *)HOR_SCHEDTO);
#else
    sema_init(&Sem, 1); /* Initialisation du sémaphore */
    proc_create("HORclock", 0, NULL, &fops_hor_clock);
    proc_create_data("HORbusy", 0, NULL, &fops_hor_fct, (void *)HOR_BUSY);
    proc_create_data("HORsched", 0, NULL, &fops_hor_fct, (void *)HOR_SCHED);
    proc_create_data("HORwait", 0, NULL, &fops_hor_fct, (void *)HOR_WAIT);
    proc_create_data("HORsched2", 0, NULL, &fops_hor_fct, (void *)HOR_SCHEDTO);
#endif
    return 0;
}

void __exit hor_exit(void)
{
    remove_proc_entry("HORclock", NULL);
    remove_proc_entry("HORbusy", NULL);
    remove_proc_entry("HORsched", NULL);
    remove_proc_entry("HORwait", NULL);
    remove_proc_entry("HORsched2", NULL);
}

module_init(hor_init);
module_exit(hor_exit);
