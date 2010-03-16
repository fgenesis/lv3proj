#ifndef COLLISIONLAYER_H
#define COLLISIONLAYER_H

// vector<bool> is a special overloaded type where each element occupies only 1 bit (C++ standard)
#ifdef _DEBUG
typedef std::vector<char> BitVector;
#else
typedef std::vector<bool> BitVector;
#endif

class BitSet2d
{
public:
    BitSet2d(uint32 xsize, uint32 ysize, bool oob)
    {
        _oob = oob;
        _xsize = xsize;
        _ysize = ysize;
        _store = new BitVector[ysize];
        for(uint32 i = 0; i < ysize; ++i)
            _store[i].resize(xsize);
    }
    ~BitSet2d()
    {
        delete [] _store;
    }
    inline void fill(bool b)
    {
        for(uint32 y = 0; y < _ysize; ++y)
        {
            BitVector& vec = _store[y];
            for(uint32 x = 0; x < _xsize; ++x)
                vec[x] = b;
        }
    }
    // cant return an L-value here since we are fiddling with bits...
    inline bool operator () (uint32 x, uint32 y)
    {
        return at(x,y);
    }
    inline void set(uint32 x, uint32 y, bool b)
    {
        DEBUG(ASSERT(y < _ysize && x < _store[y].size()));
        _store[y][x] = b;
    }
    inline bool at(uint32 x, uint32 y)
    {
        if(!(y < _ysize && x < _store[y].size()))
            return _oob;
        return _store[y][x];
    }

private:
    uint32 _xsize, _ysize;
    bool _oob;
    BitVector *_store;
};

#endif
