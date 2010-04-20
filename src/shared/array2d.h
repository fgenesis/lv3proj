#ifndef ARRAY2D_H
#define ARRAY2D_H


// fast 2D array avoiding multiplication to access array indexes
// the size has to be a power of 2, if not, it will automatically align
template <class T> class array2d
{
public:

    array2d() : _shift(0), _size(0), data(NULL) {}

    inline void fill(T val)
    {
        uint32 s = size2d();
        for(uint32 i = 0; i < s; ++i)
            data[i] = val;
    }

    inline void resize(uint32 dim, T fillval)
    {
        uint32 req = 0;
        uint32 newsize = 1;

        // find out how often we have to shift to reach the desired capacity
        // this will set the size to the nearest power of 2 required (if dim is 50 the final size will be 64, for example)
        while(newsize < dim)
        {
            newsize <<= 1;
            ++req;
        }

        // save old size and data field for later copy
        uint32 oldsize = _size;
        T* olddata = data;

        // alloc new space
        _size = newsize;
        data = new T[_size * _size];

        // fill it up to prevent uninitialized memory
        fill(fillval);

        // if there was content, copy it
        if(olddata && oldsize)
        {
            for(uint32 x = 0; x < oldsize; ++x)
                for(uint32 y = 0; y < oldsize; ++y)
                    data[(y << req) | x] = olddata[(y << _shift) | x];
            delete olddata;
        }

        _shift = req;
    }

    inline T& operator () (uint32 x, uint32 y)
    {
        return data[(y << _shift) | x];
    }

    inline T& operator [] (uint16 pos)
    {
        return data[pos];
    }

    inline T* array(void)
    {
        return &data[0];
    }

    inline uint32 size1d(void) { return _size; }
    inline uint32 size2d(void) { return _size * _size; }

    // use at your own risk
    inline T* getPtr(void) { return data; }
    inline void setPtr(T *p) { data = p; }
    inline void resizeNoAlloc(uint32 s)
    {
        _shift = 0;
        _size = 1;
        while(_size < s)
        {
            _size <<= 1;
            ++_shift;
        }
    }


protected:

    uint32 _size;
    uint32 _shift;

    T *data;

};

#endif
