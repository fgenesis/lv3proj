#include "common.h"
#include "ProgressBar.h"


ProgressBar::~ProgressBar()
{
    printf("\n");
    fflush(stdout);
}

ProgressBar::ProgressBar(uint32 total /* = 0 */)
: total(total), done(0), done2(0), _perc(0), _oldperc(0)
{
    Print();
}

void ProgressBar::Step(void)
{
    ++done;
    Update();
}

void ProgressBar::Update(void)
{
    if(total)
        _perc = ((done + done2) * 100) / total;
    else
        _perc = 0;

    if(_oldperc != _perc)
    {
        _oldperc = _perc;
        Print();
    }
}

void ProgressBar::Print(void)
{
    printf("\r[");
    uint32 i = 0, L = _perc / 2; // _perc is max 100, so L can be max. 50
    for( ; i < L; i++)
        putchar('=');
    for( ; i < 50; i++) // fill up remaining space
        putchar(' ');
    printf("] %u%% (%u/%u)\r[", _perc, done + done2, total);
    fflush(stdout);
}
