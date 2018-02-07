/**
 * Author: James
 * Git: https://github.com/jameshi16/AsyncArduino
 * 
 * Description: A small header file used to run functions almost simultaneously, on a single microcontroller like Arduino.
 **/
#ifndef ASYNC_H
#define ASYNC_H

#define MAX_FUNCTIONARRAY_SIZE 32 //Arduino Unos can only handle up to 2KB of memory, which means that the allocate() function below will freeze the Arduino if it tries to allocate too much space

/*
Function created to switch between microseconds and millseconds delay().
Note that delayMicroseconds() is accurate only up to 16383us.
*/
void wait(const unsigned long time, const bool microseconds = true) {
    if (microseconds && time > 16383) //Arduino can only accurate delay 16383 microseconds. Anything higher we have to use delay()
        delay(time / 1000);
    else if (microseconds)
        delayMicroseconds(time);
    else if (!microseconds)
        delay(time);
}

/*
The swap function. It is just more elegant to swap with a single swap() function than writing the temporary variables, and then exchanging their variables over and over
again.
*/
template <typename T>
void _swap(T& first, T& other) {
    T tmp = first;
    first = other;
    other = tmp;
}


/**
 * Function. This structure can wrap any kind of function, which is used by Async to call functions. Return value is ignored, as we are not using futures/promises (too much work for an Arduino project)
 **/
template <typename F>
struct function final {
    public:
        function()=default;
        function(F func);
        ~function();

        function(const function<F>&);
        function(function<F>&&);

        const unsigned long get_delay(bool microseconds = true) const;
        void set_delay(unsigned long delay, bool microseconds = true);

        const unsigned long getStep() const;
        void setStep(unsigned long newSize); 

        const unsigned long getId() const;
        void setId(unsigned long newId);

        void operator=(function<F>);
        const bool operator==(const function<F>&) const;
        
        void swap(function<F>&);
        
        template<typename R, class ... Tn>
        R run(Tn ... args) override;
    private:
        F m_func = nullptr; //sets the function to nullptr
        unsigned long delay_time_us = 0; //amount of time needed to be delayed
        unsigned long step = 1; //the number of steps it has done
        unsigned long id = 0; //the id of the function; useful for functions that only want the latest version of itself to run
};

/**
 * Async structure. Async allows functions to run (almost) simultaneously.
 * Permanent functions: Permanent functions will remain on the async event loop forever.
 *                      This means that for every call to run_until_complete(), permanent functions will run.
 *                      The order in which permanent functions are added is the order the functions will run sequentially within the event loop
 * Normal functions: Normal functions will be removed from the event loop after a single call to run_until_complete()
 * Reason for not using shared pointers: Most likely never going to call getAll() or getAll_Permanent().
 **/
template <typename F>
struct Async final {
public:
    Async();
    ~Async();

    Async(const Async&)=delete;
    Async(Async&&)=delete;

    void run_until_complete();
    void offsetDelayBy(unsigned long offsetDelay); //offsets all the delay in the array
    void add(function<F> fw); //adds a normal function

    void remove(int index); //removes based on index

    function<F> get(int index); //gets a function from the index
    const function<F>* getAll() const; //gets all of the functions

    int size();
    int max_size();
    void sort(); //sorts the tasks list by selection sort based on delay time within the function.
private:
    int m_size              = 1; //at least the size of 1
    int m_permsize          = 1; //size of permanent array
    int curr_size           = 0; //the current size of the tasks
    function<F> *tasks        = new function<F>[m_size]; //creates an array of functions with the size of 1
    void allocate(int newSize);
    void deallocate(int newSize);
};

/**Implementation for function**/
template <typename F>
function<F>::function(F func) {
    m_func = func;
}

template <typename F>
function<F>::~function() {
    m_func = nullptr; //makes m_func a null pointer. The function itself must continue to exist.
}

template <typename F>
function<F>::function(const function<F>& other) {
    this->m_func = other.m_func;
    this->delay_time_us = other.delay_time_us;
    this->step = other.step;
    this->id = other.id;
}

template <typename F>
function<F>::function(function<F>&& other) {
    swap(other);
}

template <typename F>
const unsigned long function<F>::get_delay(bool microseconds) const {
    if (microseconds)
        return delay_time_us;

    return delay_time_us / 1000;
}

template <typename F>
void function<F>::set_delay(unsigned long delay, bool microseconds) {
    if (microseconds) {
        delay_time_us = delay;
        return;
    }

    delay_time_us = delay * 1000;
}

template <typename F>
const unsigned long function<F>::getStep() const {
    return step;
}

template <typename F>
void function<F>::setStep(unsigned long newSize) {
    step = newSize;
}

template <typename F>
const unsigned long function<F>::getId() const {
    return id;
}

template <typename F>
void function<F>::setId(unsigned long newId) {
    id = newId;
}

template <typename F>
void function<F>::operator=(function<F> other) {
    swap(other);
}

template <typename F>
const bool function<F>::operator==(const function<F>& other) const {
    return (this->m_func == other.m_func && this->delay_time_us == other.delay_time_us && this->step == other.step && this->id == other.id);
}

template <typename F>
void function<F>::swap(function<F>& other) {
    _swap(this->m_func, other.m_func);
    _swap(this->step, other.step);
    _swap(this->delay_time_us, other.delay_time_us);
    _swap(this->id, other.id);
}

template <typename F>
template <typename R, class ... Tn>
R function<F>::run(Tn ... args) {
    return m_func(args...); //calls the function with the parameters
}

/**Implementation for Async**/
template <typename F>
Async<F>::Async() {

}

template <typename F>
Async<F>::~Async() {

}

template <>
void Async<unsigned long(*)(unsigned long, unsigned long)>::run_until_complete() {
    /* Starts the loop to complete the task list */
    while (curr_size > 0) {
        unsigned long begin = micros(); //gets the beginning time
        unsigned long returnValue = tasks[0].run<unsigned long>(tasks[0].getStep(), tasks[0].getId());
        if (returnValue > 0) {
            tasks[0].set_delay(returnValue);
            tasks[0].setStep(tasks[0].getStep() + 1); //increases the steps by 1
        }
        else remove(0); //removes the function if the return value is 0
        this->sort();

        if (curr_size == 0)
            break; //exits the loop, our size is now zero, don't read from removed functions.

        //Determines if there still needs to be a delay to the next function
        unsigned long time_spent = micros() - begin;
        if (time_spent >= tasks[0].get_delay()) {
            offsetDelayBy(time_spent); //offsets the delay
            continue; //continues the loop
        }
        else wait(tasks[0].get_delay() - time_spent);
        offsetDelayBy(tasks[0].get_delay() - time_spent); //sets all of the delays
    }
}

template <typename F>
void Async<F>::offsetDelayBy(unsigned long offsetDelay) {
    for (unsigned int iii = 0; iii < curr_size; iii++) {
        if (tasks[iii].get_delay() >= offsetDelay) //checks if the delay can be subtracted without undesirable consequence (like overflowing).
            tasks[iii].set_delay(tasks[iii].get_delay() - offsetDelay);
        else tasks[iii].set_delay(0); //sets to zero otherwise.
    }
}

template <typename F>
void Async<F>::add(function<F> fw) {
    if (curr_size >= MAX_FUNCTIONARRAY_SIZE)
        return; //return. It's game over man, it's game over.

    if (curr_size >= m_size)
        allocate(m_size * 2);

    tasks[curr_size++] = fw; //adds the fucntion into the task list
}

template <typename F>
void Async<F>::remove(int index) {
    /* Invalid Parameter checking */
    if (index >= curr_size)
        return; //Arduinos can't throw exceptions;

    if (index < 0)
        return; //it needs work continuously!
    
    function<F> temp = tasks[curr_size - 1];
    temp.swap(tasks[index]); //temp is now the object to delete

    temp.~function(); //calls the destructor for temporary
    curr_size--; //decreases the size
    this->sort();

    if (curr_size < (m_size / 2)) deallocate(m_size / 2); //deallocates memory if not needed
}

template <typename F>
function<F> Async<F>::get(int index) {
    if (index >= size)
        return tasks[curr_size - 1];

    return tasks[index];
}

template <typename F>
const function<F>* Async<F>::getAll() const {
    return tasks;
}

template <typename F>
int Async<F>::max_size() {
    return m_size;
}

template <typename F>
int Async<F>::size() {
    return curr_size;
}

template <typename F>
void Async<F>::allocate(int newSize) {
    function<F> *newTasks = new function<F>[newSize];
    if (newSize > m_size) {
        for (unsigned int iii = 0; iii < curr_size; iii++) {
            newTasks[iii] = tasks[iii];
        }
    }
    delete[] tasks; //delete tasks
    tasks = newTasks;
    m_size = newSize;
}

template <typename F>
void Async<F>::deallocate(int newSize) {
    function<F> *newTasks = new function<F>[newSize];
    for (unsigned int iii = 0; iii < newSize; iii++) {
        newTasks[iii] = tasks[iii];
    }
    delete[] tasks; //delete tasks
    tasks = newTasks;
    m_size = newSize;
}

template <typename F>
void Async<F>::sort() {
    unsigned int smallestIndex = 0;
    
    //Don't sort if the size is 0. The index used is unsigned int, so curr_size - 1 will never be achieved.
    if (curr_size == 0)
        return;

    //Selection Sort implementation
    for (unsigned int currentIndex = 0; currentIndex < curr_size - 1; currentIndex++) {
        for (unsigned int iii = currentIndex; iii < curr_size; iii++) {
            if (tasks[iii].get_delay() < tasks[smallestIndex].get_delay())
                smallestIndex = iii;
        }

        if (currentIndex != smallestIndex)
            tasks[currentIndex].swap(tasks[smallestIndex]); //swaps the two
    }
}

#endif