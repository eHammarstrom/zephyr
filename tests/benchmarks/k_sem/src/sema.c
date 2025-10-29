#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#define SEM_N 4
#define THREAD_N 8
#define NUM_LOOPS 1000
#define STACK_SIZE 1024

static struct k_sem sem[SEM_N];
static K_THREAD_STACK_ARRAY_DEFINE(thread_stack, THREAD_N, STACK_SIZE);
static struct k_thread thread_data[THREAD_N];

static void thread_fn(void *p1, void *p2, void *p3)
{
	struct k_sem *sem = (struct k_sem *)p1;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	for (int i = 0; i < NUM_LOOPS; i++) {
		k_sem_take(sem, K_FOREVER);
		k_sem_give(sem);
	}
}

ZTEST_SUITE(smp_semas, NULL, NULL, NULL, NULL, NULL);

ZTEST(smp_semas, test_smp_semas)
{
	k_tid_t threads[THREAD_N];

	for (int i = 0; i < SEM_N; i++) {
		k_sem_init(&sem[i], 1, 1);
	}

	uint32_t start_cycles = k_cycle_get_32();

	for (int i = 0; i < THREAD_N; i++) {
		threads[i] = k_thread_create(&thread_data[i], thread_stack[i],
					     STACK_SIZE, thread_fn,
					     &sem[i % SEM_N], NULL, NULL,
					     K_PRIO_COOP(3), 0, K_NO_WAIT);
	}

	for (int i = 0; i < THREAD_N; i++) {
		k_thread_join(threads[i], K_FOREVER);
	}

	uint32_t stop_cycles = k_cycle_get_32();

	uint32_t total_cycles = stop_cycles - start_cycles;

	printk("Total cycles: %u\n", total_cycles);
}


