# What is AsyncArduino?

AsyncArduino is a header file I wrote for a school project involving [Zumo](https://www.pololu.com/product/2506) robots.

Arduino has a extreme disability: It only has one microcontroller.

This means that we cannot do many things; many things at once. For instance, if I would like to operate many sensors at once, I will not be able to do it with traditional code, as reading from sensors usually (actually, most of the time) will require the use of some sort of `delay()`, which can be a great disadvantage in Zumo battle rings.

Hence, I've developed this header file that can be used in all projects made for the Arduino; not just for Zumo fighting things. This header file allows functions to run like as if they are in parallel; all within a single microcontroller.

To achieve this, we use a programming paradigm called Asynchronous, and is based on `async` found in Python. [This](https://medium.freecodecamp.org/a-guide-to-asynchronous-programming-in-python-with-asyncio-232e2afa44f6) is a Python tutorial on async, which may help you understand what I am trying to achieve here.

# Basic Usage
This header file is built to be as simple as possible.

To use `Async`, you first need to declare an object of `Async` somewhere where the object exists for the entire duration of you wanting to use `Async`:

```c++
Async<unsigned long(*)(unsigned long, unsigned long)> async;
```
Preferably, somewhere in your global variables section of the Arduino, or a shared structure passed around.

All functions used within `Async` must comply with the following template:

```c++
unsigned long a_function(unsigned long step, unsigned long id) {
    return 0; //the number here is the amount of time you want to delay in microseconds
}
```

All functions used within `Async` must then be added into the event loop as follows, in order for it to be executed:

```c++
async.add(function<unsigned long(*)(unsigned long, unsigned long)>(a_function));
```

`Async` can then be started using:
```c++
async.run_until_complete();
```

A more complete usage guide can be found [here](https://ahorribleprogrammer.wordpress.com/2018/02/07/asynchronous-on-arduino/)

# Advanced Usage
Please refer to [this](https://ahorribleprogrammer.wordpress.com/2018/02/07/asynchronous-on-arduino/) blog post I have made to understand how to comprehensively use `Async`.