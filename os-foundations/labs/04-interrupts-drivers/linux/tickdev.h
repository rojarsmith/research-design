/* SPDX-License-Identifier: GPL-2.0 */
/*
 * tickdev.h - the record that one read() of /dev/tickdev returns.
 * Shared by the module (tickdev.c) and the reader (tickread.c).
 * <linux/types.h> provides __u64 both in the kernel and in user space.
 */
#ifndef TICKDEV_H
#define TICKDEV_H

#include <linux/types.h>

struct tick_event {
	__u64 seq;          /* 1 for the first tick after loading, then 2, 3, ... */
	__u64 timestamp_ns; /* CLOCK_MONOTONIC (ktime_get_ns()) at that tick */
};

#endif
