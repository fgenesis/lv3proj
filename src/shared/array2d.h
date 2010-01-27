#ifndef ARRAY2D_H
#define ARRAY2D_H



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

        // find out how often we have to shift to reach the desired capacity capacity
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
        if(olddata)
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


protected:

    uint32 _size;
    uint32 _shift;

    T *data;

};

#endif
