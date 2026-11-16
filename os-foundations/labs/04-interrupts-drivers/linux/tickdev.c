// SPDX-License-Identifier: GPL-2.0
/*
 * tickdev.c - Stage 04, exercise D: a character device whose read()
 * blocks until the next tick of a periodic kernel timer, then returns
 * that tick's sequence number and CLOCK_MONOTONIC timestamp. A miniature
 * of the DRM event stream that stage 12 reads.
 *
 * NOT built or loaded on the machine where this lab was written: it had
 * no Linux VM, and the Linux container there shared a kernel without
 * headers. Build and load it only in a Linux VM; see the lab README.
 *
 *   make                          (uses /lib/modules/$(uname -r)/build)
 *   sudo insmod tickdev.ko period_ms=100
 *   ./tickread 10
 *   sudo rmmod tickdev
 *
 * Contexts, which the checkpoint asks about:
 *   - tick_fn() runs from the timer softirq. It must not sleep: no
 *     mutexes, no copy_to_user(), no GFP_KERNEL allocations, no msleep().
 *     It only updates two numbers under a spinlock and wakes the queue.
 *   - tick_read() runs in the reader's process context. It may sleep, and
 *     it does: in wait_event_interruptible() and possibly in copy_to_user().
 *     It never sleeps with tick_lock held.
 */
#include <linux/fs.h>
#include <linux/jiffies.h>
#include <linux/ktime.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/timer.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/wait.h>

#include "tickdev.h"

static unsigned int period_ms = 100;
module_param(period_ms, uint, 0444);
MODULE_PARM_DESC(period_ms, "tick period in milliseconds (default 100)");

static DEFINE_SPINLOCK(tick_lock); /* protects tick_seq and tick_ns */
static u64 tick_seq;               /* ticks since the module was loaded */
static u64 tick_ns;                /* ktime_get_ns() at the last tick */
static DECLARE_WAIT_QUEUE_HEAD(tick_wq);
static struct timer_list tick_timer;
static unsigned long tick_period; /* in jiffies */
static bool tick_stopping;

/* Timer softirq context: must not sleep. */
static void tick_fn(struct timer_list *unused)
{
	unsigned long flags;

	spin_lock_irqsave(&tick_lock, flags);
	tick_seq++;
	tick_ns = ktime_get_ns();
	spin_unlock_irqrestore(&tick_lock, flags);

	wake_up_interruptible(&tick_wq);

	/* Re-arm from the previous deadline, not from now, so that the
	 * period does not drift by the time it took to get here. */
	if (!READ_ONCE(tick_stopping))
		mod_timer(&tick_timer, tick_timer.expires + tick_period);
}

static u64 current_seq(void)
{
	unsigned long flags;
	u64 seq;

	spin_lock_irqsave(&tick_lock, flags);
	seq = tick_seq;
	spin_unlock_irqrestore(&tick_lock, flags);
	return seq;
}

/* Each open file remembers the last tick it returned. */
struct tick_reader {
	u64 last_seq;
};

static int tick_open(struct inode *inode, struct file *file)
{
	struct tick_reader *r = kzalloc(sizeof(*r), GFP_KERNEL);

	if (!r)
		return -ENOMEM;
	r->last_seq = current_seq(); /* the first read waits for a new tick */
	file->private_data = r;
	return stream_open(inode, file);
}

static int tick_release(struct inode *inode, struct file *file)
{
	kfree(file->private_data);
	return 0;
}

/* Process context: may sleep. */
static ssize_t tick_read(struct file *file, char __user *buf, size_t count,
			 loff_t *ppos)
{
	struct tick_reader *r = file->private_data;
	struct tick_event ev;
	unsigned long flags;
	int ret;

	if (count < sizeof(ev))
		return -EINVAL;

	if (file->f_flags & O_NONBLOCK) {
		if (current_seq() == r->last_seq)
			return -EAGAIN;
	} else {
		ret = wait_event_interruptible(tick_wq,
					       current_seq() != r->last_seq);
		if (ret)
			return ret; /* -ERESTARTSYS: a signal arrived first */
	}

	spin_lock_irqsave(&tick_lock, flags);
	ev.seq = tick_seq;
	ev.timestamp_ns = tick_ns;
	spin_unlock_irqrestore(&tick_lock, flags);
	r->last_seq = ev.seq;

	/* outside the spinlock: copy_to_user() may fault and sleep */
	if (copy_to_user(buf, &ev, sizeof(ev)))
		return -EFAULT;
	return sizeof(ev);
}

static __poll_t tick_poll(struct file *file, poll_table *wait)
{
	struct tick_reader *r = file->private_data;

	poll_wait(file, &tick_wq, wait);
	return current_seq() != r->last_seq ? EPOLLIN | EPOLLRDNORM : 0;
}

static const struct file_operations tick_fops = {
	.owner = THIS_MODULE,
	.open = tick_open,
	.release = tick_release,
	.read = tick_read,
	.poll = tick_poll,
};

/* A misc device is a character device with major number 10. Registering
 * it creates /sys/class/misc/tickdev, and udev (or devtmpfs) then makes
 * /dev/tickdev with the mode below. */
static struct miscdevice tick_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "tickdev",
	.fops = &tick_fops,
	.mode = 0444,
};

static int __init tick_init(void)
{
	int ret;

	if (period_ms == 0)
		return -EINVAL;
	tick_period = msecs_to_jiffies(period_ms);
	ret = misc_register(&tick_misc);
	if (ret)
		return ret;
	timer_setup(&tick_timer, tick_fn, 0);
	mod_timer(&tick_timer, jiffies + tick_period);
	pr_info("tickdev: /dev/%s, minor %d, one tick every %u ms (%lu jiffies at HZ=%d)\n",
		tick_misc.name, tick_misc.minor, period_ms, tick_period, HZ);
	return 0;
}

static void __exit tick_exit(void)
{
	WRITE_ONCE(tick_stopping, true);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 2, 0)
	timer_shutdown_sync(&tick_timer);
#else
	del_timer_sync(&tick_timer);
#endif
	misc_deregister(&tick_misc);
	pr_info("tickdev: unloaded after %llu ticks\n", tick_seq);
}

module_init(tick_init);
module_exit(tick_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Stage 04 lab: a timer that wakes blocked readers");
