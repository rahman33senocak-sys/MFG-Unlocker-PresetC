#include "../../source/native/dlssg_fg_lite_params.h"

#include <cassert>
#include <cstdint>
#include <map>
#include <string>

struct FakeNgxParameters
{
    std::map<std::string, uint32_t> values;

    int Get(const char* key, uint32_t* value)
    {
        const auto it = values.find(key);
        if (it == values.end()) return 1;
        *value = it->second;
        return 0;
    }
    void Set(const char* key, uint32_t value) { values[key] = value; }
};

int main()
{
    using namespace dlssg_fg_lite_params;
    assert(ShouldPin(true, 3, 3, false));
    assert(!ShouldPin(false, 3, 3, false)); // no 310.9.16/26-slot program
    assert(!ShouldPin(true, 1, 1, false));  // A untouched
    assert(!ShouldPin(true, 2, 2, false));  // B untouched
    assert(!ShouldPin(true, 3, 2, true));   // pending restart
    assert(!ShouldPin(true, 3, 3, true));   // pending restart

    FakeNgxParameters unset{};
    const auto first = Pin(unset, 0);
    assert(!first.liteBeforeValid && !first.uiBeforeValid);
    assert(first.Pinned());
    assert(unset.values.at(kLite) == 1);
    assert(unset.values.at(kUiRecomposition) == 0);

    FakeNgxParameters full{};
    full.values[kLite] = 0;
    full.values[kUiRecomposition] = 1;
    const auto override = Pin(full, 0);
    assert(override.liteBeforeValid && override.liteBefore == 0);
    assert(override.uiBeforeValid && override.uiBefore == 1);
    assert(override.Pinned());

    const auto again = Pin(full, 0);
    assert(again.liteBeforeValid && again.liteBefore == 1);
    assert(again.uiBeforeValid && again.uiBefore == 0);
    assert(again.Pinned()); // nested runtime/provider Create is idempotent
}
