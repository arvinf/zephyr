/*
 * Copyright (c) 2018 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <drivers/hwinfo.h>
#include <string.h>
#include <hal/nrf_ficr.h>
#include <sys/byteorder.h>
#include <hal/nrf_power.h>
#if NRF_POWER_HAS_RESETREAS == 0
#include <hal/nrf_reset.h>
#endif

#if (NRF_POWER_HAS_RESETREAS == 0)
#if CONFIG_SOC_NRF5340_CPUAPP || CONFIG_SOC_NRF5340_CPUNET
#define NET_RESET_REASON_MASK \
	(NRF_RESET_RESETREAS_LSREQ_MASK | NRF_RESET_RESETREAS_LLOCKUP_MASK | \
	NRF_RESET_RESETREAS_LDOG_MASK | NRF_RESET_RESETREAS_MFORCEOFF_MASK | \
	NRF_RESET_RESETREAS_LCTRLAP_MASK)
#else
#error "Unsupported SoC"
#endif
#endif

struct nrf_uid {
	uint32_t id[2];
};

ssize_t z_impl_hwinfo_get_device_id(uint8_t *buffer, size_t length)
{
	struct nrf_uid dev_id;

	dev_id.id[0] = sys_cpu_to_be32(nrf_ficr_deviceid_get(NRF_FICR, 1));
	dev_id.id[1] = sys_cpu_to_be32(nrf_ficr_deviceid_get(NRF_FICR, 0));

	if (length > sizeof(dev_id.id)) {
		length = sizeof(dev_id.id);
	}

	memcpy(buffer, dev_id.id, length);

	return length;
}


int z_impl_hwinfo_get_reset_cause(u32_t *cause)
{
	u32_t flags = 0;

#if NRF_POWER_HAS_RESETREAS
	u32_t reason = nrf_power_resetreas_get(NRF_POWER);
#else
	u32_t reason = nrf_reset_resetreas_get(NRF_RESET);
	u32_t mask = IS_ENABLED(CONFIG_SOC_NRF5340_CPUNET) ?
			NET_RESET_REASON_MASK : ~NET_RESET_REASON_MASK;
	reason &= mask;
#endif

#if NRF_POWER_HAS_RESETREAS
	if (reason & NRF_POWER_RESETREAS_RESETPIN_MASK) {
		flags |= RESET_PIN;
	}
	if (reason & NRF_POWER_RESETREAS_DOG_MASK) {
		flags |= RESET_WATCHDOG;
	}
	if (reason & NRF_POWER_RESETREAS_SREQ_MASK) {
		flags |= RESET_SOFTWARE;
	}
	if (reason & NRF_POWER_RESETREAS_LOCKUP_MASK) {
		flags |= RESET_CPU_LOCKUP;
	}
	if (reason & NRF_POWER_RESETREAS_OFF_MASK) {
		flags |= RESET_LOW_POWER_WAKE;
	}
	if (reason & NRF_POWER_RESETREAS_DIF_MASK) {
		flags |= RESET_DEBUG;
	}
#else /* NRF_POWER_HAS_RESETREAS */
#if CONFIG_SOC_NRF5340_CPUAPP
	if (reason & NRF_RESET_RESETREAS_RESETPIN_MASK) {
		flags |= RESET_PIN;
	}
	if (reason & NRF_RESET_RESETREAS_DOG0_MASK) {
		flags |= RESET_WATCHDOG;
	}
	if (reason & NRF_RESET_RESETREAS_CTRLAP_MASK) {
		flags |= RESET_DEBUG;
	}
	if (reason & NRF_RESET_RESETREAS_SREQ_MASK) {
		flags |= RESET_SOFTWARE;
	}
	if (reason & NRF_RESET_RESETREAS_LOCKUP_MASK) {
		flags |= RESET_CPU_LOCKUP;
	}
	if (reason & NRF_RESET_RESETREAS_OFF_MASK) {
		flags |= RESET_LOW_POWER_WAKE;
	}
	if (reason & NRF_RESET_RESETREAS_DIF_MASK) {
		flags |= RESET_DEBUG;
	}
	if (reason & NRF_RESET_RESETREAS_DOG1_MASK) {
		flags |= RESET_WATCHDOG;
	}
#elif CONFIG_SOC_NRF5340_CPUNET
	if (reason & NRF_RESET_RESETREAS_LSREQ_MASK) {
		flags |= RESET_SOFTWARE;
	}
	if (reason & NRF_RESET_RESETREAS_LLOCKUP_MASK) {
		flags |= RESET_CPU_LOCKUP;
	}
	if (reason & NRF_RESET_RESETREAS_LDOG_MASK) {
		flags |= RESET_WATCHDOG;
	}
	if (reason & NRF_RESET_RESETREAS_LCTRLAP_MASK) {
		flags |= RESET_DEBUG;
	}
#else
#error "Unsupported SoC"
#endif
#endif
	*cause = flags;

	return 0;
}

int z_impl_hwinfo_clear_reset_cause(void)
{
	u32_t reason = -1;

#if NRF_POWER_HAS_RESETREAS
	nrf_power_resetreas_clear(NRF_POWER, reason);
#else
	nrf_reset_resetreas_clear(NRF_RESET, reason);
#endif

	return 0;
}
