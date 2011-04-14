#include "common.h"
#include "LVPATests.h"
#include "LVPACipherTests.h"

#define DO_TESTRUN(f) { printf("Running: %s\n", #f); int _r = (f); if(_r) { logerror("TEST FAILED: Func %s returned %d", #f, _r); return 1; } }


int main(int argc, char *argv[])
{
    log_setloglevel(3);

    DO_TESTRUN(LVPATestsInit());

    DO_TESTRUN(TestLZO());
    DO_TESTRUN(TestLZMA());

    DO_TESTRUN(TestRC4());
    DO_TESTRUN(TestHPRC4Like());
    DO_TESTRUN(TestHPRC4LikeBytewise());
    DO_TESTRUN(TestRC4Warm());
    DO_TESTRUN(TestHPRC4LikeWarm());
    //DO_TESTRUN(TestRC4Massive()); // RC4 not used and just kept for reference (is rather slow too)
    DO_TESTRUN(TestHPRC4LikeMassive());

    DO_TESTRUN(TestLVPAUncompressed());
    DO_TESTRUN(TestLVPA_LZO());
    DO_TESTRUN(TestLVPA_LZMA());
    DO_TESTRUN(TestLVPAUncompressedSolid());
    DO_TESTRUN(TestLVPA_MixedSolid());
    DO_TESTRUN(TestLVPAUncompressedEncrypted());
    DO_TESTRUN(TestLVPAUncompressedScrambled());
    DO_TESTRUN(TestLVPAUncompressedEncrScram());
    DO_TESTRUN(TestLVPA_LZMA_EncrScram());
    DO_TESTRUN(TestLVPA_Everything());

    printf("All tests successful!\n");
    return 0;
}
