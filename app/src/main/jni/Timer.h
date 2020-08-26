//
// Created by Aleksey on 17.07.2018.
//

#ifndef TIMER_H
#define TIMER_H

#include<chrono>

using timepoint  = std::chrono::time_point<std::chrono::steady_clock>;
using deltaMs = std::chrono::duration<double,std::ratio<1,1000>>;
using deltaMcs = std::chrono::duration<double,std::ratio<1,1000000>>;

struct Timer {

    timepoint begin;

    inline void start()
    {
        begin = std::chrono::steady_clock::now();
    }

    double stop()
    {
        deltaMs ms;
        timepoint end = std::chrono::steady_clock::now();
        auto t = end - begin;
        ms = std::chrono::duration_cast<deltaMs>(t);
        return ms.count();
    }

};

struct scoped_timer {

    timepoint begin;
    const char* message;

    scoped_timer(const char* msg):message(msg)
    {
        begin = std::chrono::steady_clock::now();
    }

    ~scoped_timer()
    {
        deltaMs ms;
        timepoint end  = std::chrono::steady_clock::now();
        auto t = end - begin;
        ms = std::chrono::duration_cast<deltaMs>(t);
        LOG_INFO(message,"Finished within %f [ms]",ms.count());
    }

};

# 	define TIMER_CONCAT_IMPL(name, num) name##num
# 	define TIMER_CONCAT(name, num) TIMER_CONCAT_IMPL(name, num)
#   define PROFILE_SCOPE(name) scoped_timer TIMER_CONCAT(timer,__COUNTER__)(name)

#endif //EXAMPLE_TIMER_H
