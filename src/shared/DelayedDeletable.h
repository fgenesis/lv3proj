#ifndef DELAYEDDELETABLE_H
#define DELAYEDDELETABLE_H

#include <set>

class DelayedDeletable
{
public:
    DelayedDeletable() : _mustdie(false) {}
    virtual ~DelayedDeletable() {}
    virtual void SetDelete(void) { _mustdie = true; }
    virtual bool CanBeDeleted(void) { return _mustdie; }

private:
    bool _mustdie;
};

class DeletablePool
{
public:
    ~DeletablePool() { Cleanup(); }
    void Add(DelayedDeletable *d) { _store.insert(d); }
    void Cleanup(bool force = false)
    {
        for(std::set<DelayedDeletable*>::iterator it = _store.begin(); it != _store.end(); )
        {
            if(force || (*it)->CanBeDeleted())
            {
                delete *it;
                _store.erase(it++);
            }
            else
                ++it;
        }
    }

private:
    std::set<DelayedDeletable*> _store;
};

#endif
