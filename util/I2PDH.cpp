#include "I2PDH.h"

namespace i2pcpp {
    const Botan::DL_Group DH::getGroup()
    {
        const Botan::BigInt p = Botan::BigInt("0x9C05B2AA960D9B97B8931963C9CC9E8C3026E9B8ED92FAD0A69CC886D5BF8015FCADAE31A0AD18FAB3F01B00A358DE237655C4964AFAA2B337E96AD316B9FB1CC564B5AEC5B69A9FF6C3E4548707FEF8503D91DD8602E867E6D35D2235C1869CE2479C3B9D5401DE04E0727FB33D6511285D4CF29538D9E3B6051F5B22CC1C93");
        const Botan::BigInt q = Botan::BigInt("0xA5DFC28FEF4CA1E286744CD8EED9D29D684046B7");
        const Botan::BigInt g = Botan::BigInt("0xC1F4D27D40093B429E962D7223824E0BBC47E7C832A39236FC683AF84889581075FF9082ED32353D4374D7301CDA1D23C431F4698599DDA02451824FF369752593647CC3DDC197DE985E43D136CDCFC6BD5409CD2F450821142A5E6F8EB1C3AB5D0484B8129FCF17BCE4F7F33321C3CB3DBB14A905E7B2B3E93BE4708CBCC82");
        return Botan::DL_Group(p, q, g);
    }
}
