#include <scheduler.h>

//============================ STATIC VARIABLE INITIALIZATION ==========================

Scheduler Scheduler::dummy_first_task; //hook from which "real" tasks are added

//=================================== PUBLIC FUNCTIONS ==============================

//don't really need to do anything in constructor
Scheduler::Scheduler() {}

//implement a destructor that removes the particular task from the scheduled pool
//if of course it is scheduled
Scheduler::~Scheduler() {
    deschedule(); //ensure that the task can't run anymore and isn't in our pool
    //now we can just clear up all member variables using the default invoked constructor
}

//one shot event is a special case of an n-shot event
void Scheduler::schedule_oneshot_ms(Context_Callback_Function<void> _cb, uint32_t _interval_ms) {
    schedule_nshot_ms(_cb, _interval_ms, 1);
}
 
void Scheduler::schedule_nshot_ms(Context_Callback_Function<void> _cb, uint32_t _interval_ms, uint32_t count) {
    //to keep things simple, we'll start by descheduling the task
    deschedule();

    //now we'll set up everything related to n-shot
    cb = _cb;
    interval_ms = _interval_ms;
    shot_count = count;
    status = Status::RUNNING_NSHOT;

    //now grab the schedule time and hop into the list of active task (in the front of the line is easiest)
    time_of_schedule = millis();
    previous_task = &dummy_first_task; //the dummy task will be our previous task (since we're first in line)   
    next_task = dummy_first_task.next_task; //the next task in line is what's first now
    if(next_task != nullptr)            //if we have a next task...
        next_task->previous_task = this;    //the thing before our next task is us
    previous_task->next_task = this;        //the thing after our previous task is us
}

void Scheduler::schedule_interval_ms(Context_Callback_Function<void> _cb, uint32_t _interval_ms) {
    //to keep things simple, we'll start by descheduling the task
    deschedule();

    //now we'll set up everything related to continuous firing
    cb = _cb;
    interval_ms = _interval_ms;
    status = Status::RUNNING_CONTINUOUS;

    //now grab the schedule time and hop into the list of active task (in the front of the line is easiest)
    time_of_schedule = millis();
    previous_task = &dummy_first_task; //the dummy task will be our previous task (since we're first in line)   
    next_task = dummy_first_task.next_task; //the next task in line is what's first now
    if(next_task != nullptr)
        next_task->previous_task = this; //the thing before our next task is us
    previous_task->next_task = this; //the thing after our previous task is us
}

void Scheduler::deschedule() {
    //all we're really doing here is hopping outta the chain if we're in it
    //do so by setting the task before us to skip directly to our next task
    //make sure to also update the link in the backward direction
    if(status != Status::WAITING) {
        previous_task->next_task = this->next_task;
        if(next_task != nullptr)
            next_task->previous_task = this->previous_task;
    }
    //bring the task back into idle
    status = Status::WAITING;
}

void Scheduler::update() {
    //grab the first "real" task we're going to run
    Scheduler* task_to_exec = dummy_first_task.next_task;

    //go through all the active tasks
    while(task_to_exec != nullptr) { 
        task_to_exec->check_run_task();         //run the task we're pointing to
        task_to_exec = task_to_exec->next_task; //move to the next task in the linked-list
    }
}

//======================= PRIVATE MEMBER FUNCTIONS =====================

void Scheduler::check_run_task() {
    //this is the correct way to do time comparision, as it handles rollovers gracefully
    uint32_t time_of_check = millis();
    if(time_of_check - time_of_schedule >= interval_ms) {
        //reschedule our task or deschedule if our n-shot has expired
        if(status == Status::RUNNING_CONTINUOUS) 
            //use the time when we checked for better timing accuracy
            time_of_schedule = time_of_check; 
        
        
        else if (status == Status::RUNNING_NSHOT) {
            shot_count--; //decrement the number times we're executing
            if(shot_count == 0) deschedule(); //not running again
            else time_of_schedule = time_of_check; //running again
        }

        //NOTE: run the callback after normal rescheduling happens!
        //in the event that the callback function changes its scheduling, don't want to overwrite those params
        cb(); //run our function
    }
}