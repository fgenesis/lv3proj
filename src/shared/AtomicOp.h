#ifndef _ATOMIC_H
#define _ATOMIC_H

class Atomic
{
public:
    Atomic();
    ~Atomic();
    static int Incr(volatile int &i);
    static int Decr(volatile int &i);
};

#endif
