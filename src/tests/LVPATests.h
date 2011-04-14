#ifndef TESTS_LVPA_H
#define TESTS_LVPA_H

int LVPATestsInit();

int TestLZMA();
int TestLZO();
int TestLVPAUncompressed();
int TestLVPAUncompressedSolid();
int TestLVPA_LZMA();
int TestLVPA_LZO();
int TestLVPA_MixedSolid();
int TestLVPAUncompressedEncrypted();
int TestLVPAUncompressedScrambled();
int TestLVPAUncompressedEncrScram();
int TestLVPA_LZMA_EncrScram();
int TestLVPA_Everything();

#endif
