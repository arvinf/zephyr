/*
 * Copyright (c) 2018 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <syscall_handler.h>
#include <drivers/hwinfo.h>

ssize_t z_vrfy_hwinfo_get_device_id(uint8_t *buffer, size_t length)
{
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(buffer, length));

	return z_impl_hwinfo_get_device_id((uint8_t *)buffer, (size_t)length);
}
#include <syscalls/hwinfo_get_device_id_mrsh.c>

ssize_t z_vrfy_hwinfo_get_reset_cause(u32_t *cause)
{
	int ret;
	u32_t cause_copy;

	ret = z_impl_hwinfo_get_reset_cause(&cause_copy);
	Z_OOPS(z_user_to_copy(cause, &cause_copy, sizeof(u32_t)));

	return ret;
}
#include <syscalls/hwinfo_get_reset_cause_mrsh.c>


ssize_t z_vrfy_hwinfo_clear_reset_cause(void)
{
	return z_impl_hwinfo_clear_reset_cause();
}
#include <syscalls/hwinfo_clear_reset_cause_mrsh.c>
