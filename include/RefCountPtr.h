#pragma once

#include <cstdint>

#include "Macros.h"

namespace VKRT {
class RefCountPtr {
public:
    RefCountPtr();

    void AddRef();
    uint32_t Release();

protected:
    virtual ~RefCountPtr() = default;

private:
    uint32_t refCount;
};
}  // namespace VKRT