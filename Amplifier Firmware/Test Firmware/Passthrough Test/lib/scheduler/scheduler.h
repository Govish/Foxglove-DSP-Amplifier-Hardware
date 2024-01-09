#pragma once
/*
 * The world's worst scheduler
 * here to provide the most basic implementation of running tasks at specified intervals for a definite or indefinite number of times
 * Cleans up the look of our main loop a little bit, and allows us to defer the execution of particular function calls
 * 
 * This code *should* behave when instantiated on the heap (i.e. with `new`) but ideally should be statically allocated
 * 
 * By Ishaan Gov 
 */

#include <array> //container for a bunch of schedulers 
#include <Arduino.h> //types, interface

#include <config.h> //task pool size
#include <utils.h> //callback functions

class Scheduler {
public:
    Scheduler();
    ~Scheduler(); //need to implement a destructor to ensure proper behavior when delete() is called

    enum class Status {
        WAITING,
        RUNNING_NSHOT,
        RUNNING_CONTINUOUS
    };

    //set specific intervals (and counts) for which to run certain callback functions
    //NOTE: WHEN ONE OF THESE FUNCTIONS ARE CALLED MULTIPLE TIMES, IT DOESN'T QUEUE THE FUNCTION CALL MULTIPLE TIMES!
    //      Instead, it will just update when and how many time the particular function should be called
    //If it's desired to have a function execute at different intervals, create multiple `Scheduler` objects
    //      But in this case, make sure to manage memory! I recommend using `unique_ptr`s to manage heap-allocated `Scheduler`s
    void schedule_oneshot_ms(Context_Callback_Function<void> _cb, uint32_t _interval_ms);
    void schedule_nshot_ms(Context_Callback_Function<void> _cb, uint32_t _interval_ms, uint32_t count);
    void schedule_interval_ms(Context_Callback_Function<void> _cb, uint32_t _interval_ms);

    //bring a task back into the waiting state
    void deschedule();

    //call this in the main loop; basically the loop function for the scheduler
    static void update();

    //get the status/operating mode of the task
    inline Status get_status() { return status; }

private:
    //function called during update
    //checks whether task needs to run, executes the task, updates time of schedule as necessary (or deschedules task)
    void check_run_task();

    Status status = Status::WAITING; //status of the particular task, start off waiting
    uint32_t shot_count = 0; //when running in n-shot, how many more times we run
    uint32_t time_of_schedule = 0; //when the particular task was scheduled
    uint32_t interval_ms = 0; //how long between execution of different functions
    Context_Callback_Function<void> cb = Context_Callback_Function<void>(); //function we run when one-shotting

    //we'll be holding the active tasks in a linked-list style data structure
    //have pointers to the previous and next elements in our list
    Scheduler* previous_task = nullptr;
    Scheduler* next_task = nullptr;
    
    //to make the interface clean, the Scheduler class needs to own a dummy task as a permanent first element
    //this allows the scheduling/descheduling interface to be consistent no matter where in the queue tasks are added
    static Scheduler dummy_first_task;
};