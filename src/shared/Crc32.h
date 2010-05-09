#ifndef CRC32_H
#define CRC32_H


class CRC32
{
public:
    CRC32();
    void Update(uint8 *buf, uint32 size);
    void Finalize(void);
    uint32 Result(void) { return _crc; }
    static void GenTab(void);

private:
    uint32 _crc;
    static uint32 _tab[256];
};

#endif
