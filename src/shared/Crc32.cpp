#include "common.h"
#include "Crc32.h"

uint32 CRC32::_tab[256];
bool CRC32::_notab = true;

CRC32::CRC32()
: _crc(0xFFFFFFFF)
{
    if(_notab)
    {
        GenTab();
        _notab = false;
    }
}

void CRC32::GenTab(void)
{
    uint32 crc;
    for (uint16 i = 0; i < 256; i++)
    {
        crc = i;
        for (uint8 j = 8; j > 0; j--)
        {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xEDB88320L;
            else
                crc >>= 1;
        }
        _tab[i] = crc;
    }
}

void CRC32::Finalize(void)
{
    _crc ^= 0xFFFFFFFF;
}

void CRC32::Update(uint8 *buf, uint32 size)
{
    for (uint32 i = 0; i < size; i++)
        _crc = ((_crc >> 8) & 0x00FFFFFF) ^ _tab[(_crc ^ *buf++) & 0xFF];
}
