#ifndef MEMORYLEAKS_H
#define MEMORYLEAKS_H

#ifdef _DEBUG
#  define MLD SimpleMemoryLeakDetector __memleak_detector_;
#  define MLD_COUNTER SimpleMemoryLeakDetector::GetCount();
#else
#  define MLD
#  define MLD_COUNTER 0
#endif

static uint32 _counter = 0;

class SimpleMemoryLeakDetector
{
public:
    SimpleMemoryLeakDetector() { _counter++; }
    ~SimpleMemoryLeakDetector() { _counter--; }
    static uint32 GetCount(void) { return _counter; }
private:

};


#endif
