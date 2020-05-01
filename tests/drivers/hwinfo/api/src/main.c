/*
 * Copyright (c) 2019 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <drivers/hwinfo.h>
#include <ztest.h>
#include <strings.h>
#include <errno.h>

/*
 * @addtogroup t_hwinfo_get_device_id_api
 * @{
 * @defgroup t_hwinfo_get_device_id test_hwinfo_get_device_id
 * @brief TestPurpose: verify device id get works
 * @details
 * - Test Steps
 *   -# Read the ID
 *   -# Check if to many bytes are written to the buffer
 *   -# Check if UID is plausible
 * - Expected Results
 *   -# Device uid with correct length should be written to the buffer.
 * @}
 */

#define BUFFER_LENGTH 17
#define BUFFER_CANARY 0xFF

#ifdef CONFIG_HWINFO_DEVICE_ID_HAS_DRIVER
static void test_device_id_get(void)
{
	uint8_t buffer_1[BUFFER_LENGTH];
	uint8_t buffer_2[BUFFER_LENGTH];
	ssize_t length_read_1, length_read_2;
	int i;

	length_read_1 = hwinfo_get_device_id(buffer_1, 1);
	zassert_not_equal(length_read_1, -ENOTSUP, "Not supported by hardware");
	zassert_false((length_read_1 < 0),
		      "Unexpected negative return value: %d", length_read_1);
	zassert_not_equal(length_read_1, 0, "Zero bytes read");
	zassert_equal(length_read_1, 1, "Length not adhered");

	memset(buffer_1, BUFFER_CANARY, sizeof(buffer_1));

	length_read_1 = hwinfo_get_device_id(buffer_1, BUFFER_LENGTH - 1);
	zassert_equal(buffer_1[length_read_1], BUFFER_CANARY,
		      "Too many bytes are written");

	memcpy(buffer_2, buffer_1, length_read_1);

	for (i = 0; i < BUFFER_LENGTH; i++) {
		buffer_1[i] ^= 0xA5;
	}

	length_read_2 = hwinfo_get_device_id(buffer_1, BUFFER_LENGTH - 1);
	zassert_equal(length_read_1, length_read_2, "Length varied");

	zassert_equal(buffer_1[length_read_1], (BUFFER_CANARY ^ 0xA5),
		      "Too many bytes are written");

	for (i = 0; i < length_read_1; i++) {
		zassert_equal(buffer_1[i], buffer_2[i],
			      "Two consecutively readings don't match");
	}
}
#else
static void test_device_id_get(void) {}
#endif /* CONFIG_HWINFO_DEVICE_ID_HAS_DRIVER */

#ifndef CONFIG_HWINFO_DEVICE_ID_HAS_DRIVER
static void test_device_id_enotsup(void)
{
	ssize_t ret;
	uint8_t buffer[1];

	ret = hwinfo_get_device_id(buffer, 1);
	/* There is no hwinfo driver for this platform, hence the return value
	 * should be -ENOTSUP
	 */
	zassert_equal(ret, -ENOTSUP,
		      "hwinfo_get_device_id returned % instead of %d",
		      ret, -ENOTSUP);
}
#else
static void test_device_id_enotsup(void) {}
#endif /* CONFIG_HWINFO_DEVICE_ID_HAS_DRIVER not defined*/

/*
 * @addtogroup t_hwinfo_get_reset_cause_api
 * @{
 * @defgroup t_hwinfo_get_reset_cause test_hwinfo_get_reset_cause
 * @brief TestPurpose: verify get reset cause works.
 * @details
 * - Test Steps
 *   -# Set target buffer to a known value
 *   -# Read the reset cause
 *   -# Check if target buffer has been altered
 * - Expected Results
 *   -# Target buffer contents should be changed after the call.
 * @}
 */
#ifdef CONFIG_HWINFO_RESET_CAUSE_HAS_DRIVER
static void test_get_reset_cause(void)
{
	u32_t cause;
	ssize_t ret;

	/* Set `cause` to a known value prior to call. */
	cause = 0xDEADBEEF;

	ret = hwinfo_get_reset_cause(&cause);
	zassert_not_equal(ret, -ENOTSUP, "Not supported by hardware");
	zassert_false((ret < 0),
		      "Unexpected negative return value: %d", ret);

	/* Verify that `cause` has been changed. */
	zassert_not_equal(cause, 0xDEADBEEF, "Reset cause not written.");
}
#else
static void test_get_reset_cause(void) {}
#endif /* CONFIG_HWINFO_RESET_CAUSE_HAS_DRIVER */

#ifndef CONFIG_HWINFO_RESET_CAUSE_HAS_DRIVER
static void test_get_reset_cause_enotsup(void)
{
	ssize_t ret;
	u32_t cause;

	ret = hwinfo_get_reset_cause(&cause);
	/* There is no hwinfo driver for this platform, hence the return value
	 * should be -ENOTSUP
	 */
	zassert_equal(ret, -ENOTSUP,
		      "hwinfo_get_reset_cause returned % instead of %d",
		      ret, -ENOTSUP);
}
#else
static void test_get_reset_cause_enotsup(void) {}
#endif /* CONFIG_HWINFO_RESET_CAUSE_HAS_DRIVER not defined*/


/*
 * @addtogroup t_hwinfo_clear_reset_cause_api
 * @{
 * @defgroup t_hwinfo_clear_reset_cause test_hwinfo_clear_reset_cause
 * @brief TestPurpose: verify clear reset cause works. This may
 *        not work on some platforms, depending on how reset cause register
 *        works on that SoC.
 * @details
 * - Test Steps
 *   -# Read the reset cause and store the result
 *   -# Call clear reset cause
 *   -# Read the reset cause again
 *   -# Check if the two readings match
 * - Expected Results
 *   -# Reset cause value should change after calling clear reset cause.
 * @}
 */
#ifdef CONFIG_HWINFO_RESET_CAUSE_HAS_DRIVER
static void test_clear_reset_cause(void)
{
	u32_t cause_1, cause_2;
	ssize_t ret;

	ret = hwinfo_get_reset_cause(&cause_1);
	zassert_not_equal(ret, -ENOTSUP, "Not supported by hardware");
	zassert_false((ret < 0),
		      "Unexpected negative return value: %d", ret);

	ret = hwinfo_clear_reset_cause();
	zassert_not_equal(ret, -ENOTSUP, "Not supported by hardware");
	zassert_false((ret < 0),
		      "Unexpected negative return value: %d", ret);

	ret = hwinfo_get_reset_cause(&cause_2);
	zassert_not_equal(ret, -ENOTSUP, "Not supported by hardware");
	zassert_false((ret < 0),
		      "Unexpected negative return value: %d", ret);

	/* Verify that `cause` has been changed. */
	zassert_not_equal(cause_1, cause_2,
		"Reset cause did not change after clearing");
}
#else
static void test_clear_reset_cause(void) {}
#endif /* CONFIG_HWINFO_RESET_CAUSE_HAS_DRIVER */

#ifndef CONFIG_HWINFO_RESET_CAUSE_HAS_DRIVER
static void test_clear_reset_cause_enotsup(void)
{
	ssize_t ret;

	ret = hwinfo_clear_reset_cause();
	/* There is no hwinfo driver for this platform, hence the return value
	 * should be -ENOTSUP
	 */
	zassert_equal(ret, -ENOTSUP,
		      "hwinfo_clear_reset_cause returned % instead of %d",
		      ret, -ENOTSUP);
}
#else
static void test_clear_reset_cause_enotsup(void) {}
#endif /* CONFIG_HWINFO_RESET_CAUSE_HAS_DRIVER not defined*/

void test_main(void)
{
	ztest_test_suite(hwinfo_device_id_api,
			 ztest_unit_test(test_device_id_get),
			 ztest_unit_test(test_device_id_enotsup),
			 ztest_unit_test(test_get_reset_cause),
			 ztest_unit_test(test_get_reset_cause_enotsup),
			 ztest_unit_test(test_clear_reset_cause),
			 ztest_unit_test(test_clear_reset_cause_enotsup)
			);

	ztest_run_test_suite(hwinfo_device_id_api);
}
