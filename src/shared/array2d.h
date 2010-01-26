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

    inline void resize(uint32 dim)
    {
        uint32 req = 0;
        uint32 n = dim >> 1;
        while(n)
        {
            n >>= 1;
            ++req;
        }
        if((uint32(1) << req) < dim)
            req++;

        bool mustcopy = _size || data;

        _size = 1 << req;
        if(dim % _size)
            req++;
        

        T *newdata = new T[_size * _size];

        if(mustcopy)
        {
            uint32 maxdim = std::max(_size, dim);
            for(uint32 x = 0; x < maxdim; ++x)
                for(uint32 y = 0; y < maxdim; ++y)
                    newdata[(y << req) | x] = data[(y << _shift) | x];
            delete data;
        }

        data = newdata;

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
