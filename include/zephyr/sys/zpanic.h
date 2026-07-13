/*
 * Copyright (c) 2026 Emil Hammarström <emil.a.hammarstrom@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief More expressive panics depending on subsystem assertion level.
 * @ingroup sys_check_apis
 */

#ifndef ZEPHYR_INCLUDE_SYS_PANIC_H_
#define ZEPHYR_INCLUDE_SYS_PANIC_H_

#include <zephyr/sys/zassert.h>

/**
 * @defgroup sys_panic_apis Panic handling
 * @ingroup os_services
 * @{
 */

/**
 * @brief Assert if applicable for the current subsystem assertion level, panic if assertions
 *        are disabled.
 *
 * Provides an expressive assert when assertions are enabled for the current assert group, and a
 * panic if assertions are disabled.
 *
 * Before,
 *
 * @code{.c}
 * if (ret != 0) {
 * 	LOG_ERR("Could not set thread %p shadow stack %p, got error %d",
 * 	new_thread, stk_to_hw_shstk->shstk_addr, ret);
 * 	k_panic();
 * }
 * @endcode
 *
 * After,
 *
 * @code{.c}
 * ZPANIC(ret != 0, "Could not set thread %p shadow stack %p, got error %d",
 * 	new_thread, stk_to_hw_shstk->shstk_addr, ret);
 * @endcode
 *
 * The compile time behavior is inherited from @ref zassert groups, and the runtime behavior is
 * a superset of ZASSERT and k_panic.
 *
 * @param expr Panic condition, if true the system will assert and panic.
 * @param ... Optional formatted string message followed by its arguments. May be omitted.
 */
#define ZPANIC(expr, ...) \
    ZASSERT_IF(expr, __VA_ARGS__) \
    if (expr) k_panic()

/** @} */

#endif /* ZEPHYR_INCLUDE_SYS_PANIC_H_ */
