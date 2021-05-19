/*
 * Copyright (c) 2021 IP-Logix Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT atmel_sam_mdio

#include <errno.h>
#include <device.h>
#include <init.h>
#include <soc.h>
#include <drivers/mdio.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(mdio_sam, CONFIG_MDIO_LOG_LEVEL);

/* GMAC */
#ifdef CONFIG_SOC_FAMILY_SAM0
#define GMAC_MAN        MAN.reg
#define GMAC_NSR        NSR.reg
#define GMAC_NCR        NCR.reg
#endif

struct mdio_sam_dev_data {
	struct k_sem sem;
};

struct mdio_sam_dev_config {
	Gmac * const regs;
	int protocol;
};

#define DEV_NAME(dev) ((dev)->name)
#define DEV_DATA(dev) ((struct mdio_sam_dev_data *const)(dev)->data)
#define DEV_CFG(dev) \
	((const struct mdio_sam_dev_config *const)(dev)->config)

static int mdio_transfer(const struct device *dev, uint8_t prtad,
			  uint8_t devad, uint8_t rw, uint16_t data_in,
			  uint16_t *data_out)
{
	const struct mdio_sam_dev_config *const cfg = DEV_CFG(dev);
	struct mdio_sam_dev_data *const data = DEV_DATA(dev);
	int timeout = 50;

	k_sem_take(&data->sem, K_FOREVER);

	cfg->regs->GMAC_NCR |= GMAC_NCR_MPE;

	/* Write mdio transaction */
	if (cfg->protocol == CLAUSE_45) {
		cfg->regs->GMAC_MAN = (GMAC_MAN_OP(rw ? 0x2 : 0x3))
				     | GMAC_MAN_WTN(0x02)
				     | GMAC_MAN_PHYA(prtad)
				     | GMAC_MAN_REGA(devad)
				     | GMAC_MAN_DATA(data_in);
	} else if (cfg->protocol == CLAUSE_22) {
		cfg->regs->GMAC_MAN =  GMAC_MAN_CLTTO
				     | (GMAC_MAN_OP(rw ? 0x2 : 0x1))
				     | GMAC_MAN_WTN(0x02)
				     | GMAC_MAN_PHYA(prtad)
				     | GMAC_MAN_REGA(devad)
				     | GMAC_MAN_DATA(data_in);


	} else {
		LOG_ERR("Unsupported protocol");
	}

	/* Wait until done */
	while (!(cfg->regs->GMAC_NSR & GMAC_NSR_IDLE)) {
		if (timeout-- == 0U) {
			LOG_ERR("transfer timedout %s", DEV_NAME(dev));
			k_sem_give(&data->sem);
			return -ETIMEDOUT;
		}

		k_sleep(K_MSEC(5));
	}

	if (data_out) {
		*data_out = cfg->regs->GMAC_MAN & GMAC_MAN_DATA_Msk;
	}

	k_sem_give(&data->sem);
	return 0;
}

static int mdio_sam_read(const struct device *dev, uint8_t prtad, uint8_t devad,
			 uint16_t *data)
{
	return mdio_transfer(dev, prtad, devad, 1, 0, data);
}

static int mdio_sam_write(const struct device *dev, uint8_t prtad,
			  uint8_t devad, uint16_t data)
{
	return mdio_transfer(dev, prtad, devad, 0, data, NULL);
}

static void mdio_sam_bus_enable(const struct device *dev)
{
	const struct mdio_sam_dev_config *const cfg = DEV_CFG(dev);

	cfg->regs->GMAC_NCR |= GMAC_NCR_MPE;
}

static void mdio_sam_bus_disable(const struct device *dev)
{
	const struct mdio_sam_dev_config *const cfg = DEV_CFG(dev);

	cfg->regs->GMAC_NCR &= ~GMAC_NCR_MPE;
}

#ifdef CONFIG_SOC_FAMILY_SAM0
#define MCK_FREQ_HZ	SOC_ATMEL_SAM0_MCK_FREQ_HZ
#elif CONFIG_SOC_FAMILY_SAM
#define MCK_FREQ_HZ	SOC_ATMEL_SAM_MCK_FREQ_HZ
#else
#error Unsupported SoC family
#endif

static int get_mck_clock_divisor(uint32_t mck)
{
	uint32_t mck_divisor;

	if (mck <= 20000000U) {
		mck_divisor = GMAC_NCFGR_CLK_MCK_8;
	} else if (mck <= 40000000U) {
		mck_divisor = GMAC_NCFGR_CLK_MCK_16;
	} else if (mck <= 80000000U) {
		mck_divisor = GMAC_NCFGR_CLK_MCK_32;
	} else if (mck <= 120000000U) {
		mck_divisor = GMAC_NCFGR_CLK_MCK_48;
	} else if (mck <= 160000000U) {
		mck_divisor = GMAC_NCFGR_CLK_MCK_64;
	} else if (mck <= 240000000U) {
		mck_divisor = GMAC_NCFGR_CLK_MCK_96;
	} else {
		LOG_ERR("No valid MDC clock");
		mck_divisor = -ENOTSUP;
	}

	return mck_divisor;
}

#ifdef CONFIG_SOC_FAMILY_SAM
static const struct soc_gpio_pin pins_eth0[] = ATMEL_SAM_DT_INST_PINS(0);
#endif

static int mdio_sam_initialize(const struct device *dev)
{
	const struct mdio_sam_dev_config *const cfg = DEV_CFG(dev);
	struct mdio_sam_dev_data *const data = DEV_DATA(dev);

	k_sem_init(&data->sem, 1, 1);


#ifdef CONFIG_SOC_FAMILY_SAM
	/* Enable GMAC module's clock */
	soc_pmc_peripheral_enable(ID_GMAC);

	soc_gpio_list_configure(pins_eth0, ARRAY_SIZE(pins_eth0));
#else
	/* Enable MCLK clock on GMAC */
	MCLK->AHBMASK.reg |= MCLK_AHBMASK_GMAC;
	*MCLK_GMAC |= MCLK_GMAC_MASK;
#endif

	int mck_divisor;

	mck_divisor = get_mck_clock_divisor(MCK_FREQ_HZ);
	if (mck_divisor < 0) {
		return mck_divisor;
	}

	LOG_INF("MCLK: %d, mck_divisor: %d", MCK_FREQ_HZ, mck_divisor);
	LOG_INF("GMAC_NCFGR: %08x", cfg->regs->GMAC_NCFGR);

	/* Set Network Control Register to Enable MPE */
	cfg->regs->GMAC_NCR |= GMAC_NCR_MPE;

	/* Disable all interrupts */
	cfg->regs->GMAC_IDR = UINT32_MAX;
	/* Clear all interrupts */
	(void)cfg->regs->GMAC_ISR;

	cfg->regs->GMAC_NCFGR = mck_divisor;
	LOG_INF("GMAC_NCFGR: %08x", cfg->regs->GMAC_NCFGR);

	return 0;
}

static const struct mdio_driver_api mdio_sam_driver_api = {
	.read = mdio_sam_read,
	.write = mdio_sam_write,
	.bus_enable = mdio_sam_bus_enable,
	.bus_disable = mdio_sam_bus_disable,
};

#define MDIO_SAM_CONFIG(n)						\
static const struct mdio_sam_dev_config mdio_sam_dev_config_##n = {	\
	.regs = (Gmac *)DT_INST_REG_ADDR(n),				\
	.protocol = DT_ENUM_IDX(DT_DRV_INST(n), protocol),		\
};

#define MDIO_SAM_DEVICE(n)						\
	MDIO_SAM_CONFIG(n);						\
	static struct mdio_sam_dev_data mdio_sam_dev_data##n;		\
	DEVICE_DT_INST_DEFINE(n,					\
			    &mdio_sam_initialize,			\
			    NULL,					\
			    &mdio_sam_dev_data##n,			\
			    &mdio_sam_dev_config_##n, POST_KERNEL,	\
			    CONFIG_MDIO_INIT_PRIORITY,			\
			    &mdio_sam_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MDIO_SAM_DEVICE)
