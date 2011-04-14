#ifndef MYCRC32_H
#define MYCRC32_H


class CRC32
{
public:
    CRC32();
    void Update(const uint8 *buf, uint32 size);
    void Finalize(void);
    uint32 Result(void) { return _crc; }

    inline static uint32 Calc(const uint8 *buf, uint32 size)
    {
        CRC32 crc;
        crc.Update(buf, size);
        crc.Finalize();
        return crc.Result();
    }

private:
    static void GenTab(void);
    uint32 _crc;
    static uint32 _tab[256];
    static bool _notab;
};

#endif
