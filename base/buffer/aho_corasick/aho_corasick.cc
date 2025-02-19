#include "aho_corasick.h"

#include "ac_fast.h"
#include "ac_slow.h"

#include <fmt/base.h>

namespace base {

class BufAlloc : public BufAllocator {
public:
    virtual ACBuffer* alloc(int sz) {
        return (ACBuffer*)(new unsigned char[sz]);
    }

    // Do not de-allocate the buffer when the BufAlloc die.
    virtual void free() {}

    static void myfree(ACBuffer* buf) {
        const char* b = (const char*)buf;
        delete[] b;
    }
};

AhoCorasick::AhoCorasick(const std::vector<std::string>& patterns) {
    if (patterns.size() >= 65535) {
        // TODO: Currently we use 16-bit to encode pattern-index (see the comment to
        // AC_State::is_term), therefore we are not able to handle pattern set with more than 65535
        // entries.
        fmt::println("Error: Pattern limit of 65535 exceeded in AhoCorasick constructor.");
        std::abort();
    }

    ACSlowConstructor acc;
    acc.Construct(patterns);

    BufAlloc ba;
    ACConverter cvt(acc, ba);
    ACBuffer* buf = cvt.Convert();
    this->buf = (void*)buf;
}

AhoCorasick::~AhoCorasick() {
    ACBuffer* buf = (ACBuffer*)this->buf;
    BufAlloc::myfree(buf);
}

AhoCorasick::MatchResult AhoCorasick::match(const PieceTree& tree) const {
    ACBuffer* buf = (ACBuffer*)this->buf;
    return Match(buf, tree);
}

}  // namespace base
