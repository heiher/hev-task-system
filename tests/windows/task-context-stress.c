/* Regression test for register/stack preservation across many task yields.
 * Especially important for the separate native MSVC x86/x64 ASM backends. */
#include <assert.h>
#include <stddef.h>
#include <hev-task.h>
#include <hev-task-system.h>

#define TASKS 12
#define SWITCHES 200

typedef struct
{
    unsigned int index;
    unsigned int completed;
} Work;

static void
worker (void *arg)
{
    Work *w = arg;
    volatile unsigned char guard[64];
    unsigned int round;
    unsigned int i;

    for (i = 0; i < sizeof (guard); ++i)
        guard[i] = (unsigned char)(i ^ w->index);

    for (round = 0; round < SWITCHES; ++round) {
        hev_task_yield (HEV_TASK_YIELD);
        for (i = 0; i < sizeof (guard); ++i)
            assert (guard[i] == (unsigned char)(i ^ w->index));
        ++w->completed;
    }
}

int
main (void)
{
    Work work[TASKS] = { { 0 } };
    unsigned int i;

    assert (hev_task_system_init () == 0);
    for (i = 0; i < TASKS; ++i) {
        HevTask *task = hev_task_new (-1);
        assert (task != NULL);
        work[i].index = i;
        hev_task_run (task, worker, &work[i]);
    }
    hev_task_system_run ();
    for (i = 0; i < TASKS; ++i)
        assert (work[i].completed == SWITCHES);
    hev_task_system_fini ();
    return 0;
}
