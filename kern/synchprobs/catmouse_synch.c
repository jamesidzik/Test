#include <types.h>
#include <lib.h>
#include <synchprobs.h>
#include <synch.h>


/* 
 * Replace this default synchronization mechanism with your own (better) mechanism
 * needed for your solution.   Your mechanism may use any of the available synchronzation
 * primitives, e.g., semaphores, locks, condition variables.   You are also free to 
 * declare other global variables if your solution requires them.
 */


static struct lock **bowlLocks;
static struct cv *canCatsEat;
static struct cv *canMiceEat;
static volatile int catsEating;
static volatile int miceEating;
static struct lock *counterLock;
static struct lock *startingEating;

/* 
 * The CatMouse simulation will call this function once before any cat or
 * mouse tries to each.
 *
 * You can use it to initialize synchronization and other variables.
 * 
 * parameters: the number of bowls
 */
void
catmouse_sync_init(int bowls)
{
    bowlLocks = kmalloc(sizeof(struct lock *) * bowls);
    if (bowlLocks == NULL)
    {
        panic("could not create lock array for CatMouse simulation");
    }
    
    for (int i = 0; i < bowls; i++)
    {
        bowlLocks[i] = lock_create("Bowl lock");
        if (bowlLocks[i] == NULL)
        {
            panic("could not create lock for CatMouse simulation");
        }
    }
 
    canCatsEat = cv_create("Cat Control Variable");
    canMiceEat = cv_create("Mouse Control Variable");
    counterLock = lock_create("Counter Lock");
    startingEating = lock_create("Initial Lock");
    catsEating = 0;
    miceEating = 0;

    return;
}

/* 
 * The CatMouse simulation will call this function once after all cat
 * and mouse simulations are finished.
 *
 * You can use it to clean up any synchronization and other variables.
 *
 * parameters: the number of bowls
 */
void
catmouse_sync_cleanup(int bowls)
{
    for(int i = 0; i < bowls; i++)
    {
        lock_destroy(bowlLocks[i]);
    }
    kfree(bowlLocks);
    cv_destroy(canCatsEat);
    cv_destroy(canMiceEat);
}


/*
 * The CatMouse simulation will call this function each time a cat wants
 * to eat, before it eats.
 * This function should cause the calling thread (a cat simulation thread)
 * to block until it is OK for a cat to eat at the specified bowl.
 *
 * parameter: the number of the bowl at which the cat is trying to eat
 *             legal bowl numbers are 1..NumBowls
 *
 * return value: none
 */

void
cat_before_eating(unsigned int bowl) 
{
    lock_acquire(bowlLocks[bowl-1]);
    lock_acquire(counterLock);
    while (miceEating > 0)
    {
        lock_release(counterLock);
        cv_wait(canCatsEat, bowlLocks[bowl-1]);
        lock_acquire(counterLock);
    }

    catsEating = catsEating + 1;
    lock_release(counterLock);
}

/*
 * The CatMouse simulation will call this function each time a cat finishes
 * eating.
 *
 * You can use this function to wake up other creatures that may have been
 * waiting to eat until this cat finished.
 *
 * parameter: the number of the bowl at which the cat is finishing eating.
 *             legal bowl numbers are 1..NumBowls
 *
 * return value: none
 */

void
cat_after_eating(unsigned int bowl) 
{
    lock_acquire(counterLock);
    catsEating = catsEating - 1;
    if (catsEating == 0)
    {
        cv_broadcast(canMiceEat, bowlLocks[bowl-1]);
    }
    lock_release(counterLock);

    lock_release(bowlLocks[bowl-1]);
}

/*
 * The CatMouse simulation will call this function each time a mouse wants
 * to eat, before it eats.
 * This function should cause the calling thread (a mouse simulation thread)
 * to block until it is OK for a mouse to eat at the specified bowl.
 *
 * parameter: the number of the bowl at which the mouse is trying to eat
 *             legal bowl numbers are 1..NumBowls
 *
 * return value: none
 */

void
mouse_before_eating(unsigned int bowl) 
{
    lock_acquire(bowlLocks[bowl-1]);
    lock_acquire(counterLock);
    while(catsEating > 0)
    {
        lock_release(counterLock);
        cv_wait(canMiceEat, bowlLocks[bowl-1]);
        lock_acquire(counterLock);
    }
    
    miceEating = miceEating + 1;
    lock_release(counterLock);
}

/*
 * The CatMouse simulation will call this function each time a mouse finishes
 * eating.
 *
 * You can use this function to wake up other creatures that may have been
 * waiting to eat until this mouse finished.
 *
 * parameter: the number of the bowl at which the mouse is finishing eating.
 *             legal bowl numbers are 1..NumBowls
 *
 * return value: none
 */

void
mouse_after_eating(unsigned int bowl) 
{
    lock_acquire(counterLock);
    miceEating = miceEating - 1;
    if (miceEating == 0)
    {
        cv_broadcast(canCatsEat, bowlLocks[bowl-1]);
    }
    lock_release(counterLock);

    lock_release(bowlLocks[bowl-1]);
}
