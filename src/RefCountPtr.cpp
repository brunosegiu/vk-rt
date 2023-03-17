#include "RefCountPtr.h"

namespace VKRT {
RefCountPtr::RefCountPtr() : refCount(1) {}

void RefCountPtr::AddRef() {
    ++refCount;
}

uint32_t RefCountPtr::Release() {
    refCount--;
    if (refCount == 0) {
        delete this;
        return 0;
    } else {
        return refCount;
    }
}

}  // namespace VKRT
