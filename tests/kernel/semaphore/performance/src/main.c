/*
 * Copyright (c) 2025 Måns Ansgariusson <mansgariusson@gmail.com>
 *                    Emil Hammarström <emil.a.hammarstrom@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/logging/log.h>

#define THREAD_N (CONFIG_MP_MAX_NUM_CPUS > 1 ? CONFIG_MP_MAX_NUM_CPUS : 1)
#define NUM_LOOPS 1000
#define STACK_SIZE 2048

static struct k_thread thread_data[THREAD_N];
static K_THREAD_STACK_ARRAY_DEFINE(thread_stack, THREAD_N, STACK_SIZE);

LOG_MODULE_REGISTER(semaphore_performance, LOG_LEVEL_INF);


static void thread_fn(void *p1, void *p2, void *p3)
{
	struct k_sem *sem = (struct k_sem *)p1;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	for (int i = 0; i < NUM_LOOPS; i++) {
		int rc;

		do {
			rc = k_sem_take(sem, K_NO_WAIT);
		} while (rc != 0);

		k_sem_give(sem);
	}
}

ZTEST(semaphore_performance, test_multi_thread_m_m)
{
	struct k_sem sems[THREAD_N];
	k_tid_t threads[THREAD_N];
	uint32_t start_cycles, total_cycles;

	for (size_t i = 0; i < ARRAY_SIZE(sems); i++) {
		k_sem_init(&sems[i], 1, 1);
	}

	start_cycles = k_cycle_get_32();

	for (size_t i = 0; i < ARRAY_SIZE(threads); i++) {
		threads[i] = k_thread_create(&thread_data[i],
				thread_stack[i], STACK_SIZE,
				thread_fn, &sems[i], NULL, NULL,
				K_PRIO_COOP(3), 0, K_NO_WAIT);
	}

	for (size_t i = 0; i < THREAD_N; i++) {
		k_thread_join(threads[i], K_FOREVER);
	}

	total_cycles = k_cycle_get_32() - start_cycles;
	LOG_INF("Total cycles: %u", total_cycles);
}

ZTEST(semaphore_performance, test_multi_thread_m_n)
{
	struct k_sem sems[CONFIG_MP_MAX_NUM_CPUS > 1 ? CONFIG_MP_MAX_NUM_CPUS / 2 : 1];
	k_tid_t threads[THREAD_N];
	uint32_t start_cycles, total_cycles;

	for (size_t i = 0; i < ARRAY_SIZE(sems); i++) {
		k_sem_init(&sems[i], 1, 1);
	}

	start_cycles = k_cycle_get_32();

	for (size_t i = 0; i < THREAD_N; i++) {
		threads[i] = k_thread_create(&thread_data[i],
				thread_stack[i], STACK_SIZE,
				thread_fn, &sems[i % ARRAY_SIZE(sems)], NULL, NULL,
				K_PRIO_COOP(3), 0, K_NO_WAIT);
	}

	for (size_t i = 0; i < THREAD_N; i++) {
		k_thread_join(threads[i], K_FOREVER);
	}

	total_cycles = k_cycle_get_32() - start_cycles;
	LOG_INF("Total cycles: %u", total_cycles);
}

ZTEST(semaphore_performance, test_multi_thread_m_1)
{
	struct k_sem sem;
	k_tid_t threads[THREAD_N];
	uint32_t start_cycles, total_cycles;

	k_sem_init(&sem, 1, 1);

	start_cycles = k_cycle_get_32();

	for (size_t i = 0; i < THREAD_N; i++) {
		threads[i] = k_thread_create(&thread_data[i],
				thread_stack[i], STACK_SIZE,
				thread_fn, &sem, NULL, NULL,
				K_PRIO_COOP(3), 0, K_NO_WAIT);
	}

	for (size_t i = 0; i < THREAD_N; i++) {
		k_thread_join(threads[i], K_FOREVER);
	}

	total_cycles = k_cycle_get_32() - start_cycles;
	LOG_INF("Total cycles: %u", total_cycles);
}

ZTEST(semaphore_performance, test_single_thread)
{
	uint32_t start_cycles, total_cycles;
	struct k_sem sem;

	k_sem_init(&sem, 1, 1);
	start_cycles = k_cycle_get_32();

	for (size_t i = 0; i < NUM_LOOPS; i++) {
		k_sem_take(&sem, K_FOREVER);
		k_sem_give(&sem);
	}

	total_cycles = k_cycle_get_32() - start_cycles;
	LOG_INF("Total cycles: %u", total_cycles);
}


ZTEST_SUITE(semaphore_performance, NULL, NULL, NULL, NULL, NULL);
