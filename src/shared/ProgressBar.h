#ifndef PROGRESSBAR_H
#define PROGRESSBAR_H

class ProgressBar
{
public:
    void Step(void);
    void Update(void);
    void Print(void);
    inline void PartialFix(void) { done2 += done; done = 0; Update(); }
    inline void Finalize(void) { done2 = 0; done = total; _perc = 100; Print(); }
    ProgressBar(uint32 total = 0);
    ~ProgressBar();

    uint32 total, done, done2;
    
private:
    uint32 _perc, _oldperc;

};

#endif
