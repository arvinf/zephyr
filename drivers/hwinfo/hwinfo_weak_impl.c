/*
 * Copyright (c) 2018 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/hwinfo.h>

ssize_t __weak z_impl_hwinfo_get_device_id(uint8_t *buffer, size_t length)
{
	return -ENOTSUP;
}

int __weak z_impl_hwinfo_get_reset_cause(u32_t *cause)
{
	return -ENOTSUP;
}

int __weak z_impl_hwinfo_clear_reset_cause(void)
{
	return -ENOTSUP;
}
