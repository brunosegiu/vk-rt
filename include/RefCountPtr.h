#pragma once

#include <cstdint>
#include <memory>

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

template <class T>
class ScopedRefPtr {
public:
    ScopedRefPtr() : mPtr(nullptr) {}

    ScopedRefPtr(T* ptr) : mPtr(ptr) {
        if (mPtr != nullptr) {
            mPtr->AddRef();
        }
    }

    ScopedRefPtr(const ScopedRefPtr<T>& r) : mPtr(r.mPtr) {
        if (mPtr != nullptr) {
            mPtr->AddRef();
        }
    }

    template <typename U>
    ScopedRefPtr(const ScopedRefPtr<U>& r) : mPtr(r.Get()) {
        if (mPtr != nullptr) {
            mPtr->AddRef();
        }
    }

    T* Get() const { return mPtr; }
    operator T*() const { return mPtr; }
    T& operator*() const { return *mPtr; }
    T* operator->() const { return mPtr; }

    T* Release() {
        T* retVal = mPtr;
        mPtr = nullptr;
        return retVal;
    }

    ScopedRefPtr<T>& operator=(T* ptr) {
        if (ptr != nullptr) {
            ptr->AddRef();
        }
        if (mPtr != nullptr) {
            mPtr->Release();
        }
        mPtr = ptr;
        return *this;
    }

    ScopedRefPtr<T>& operator=(const ScopedRefPtr<T>& r) { return *this = r.mPtr; }

    template <typename U>
    ScopedRefPtr<T>& operator=(const ScopedRefPtr<U>& r) {
        return *this = r.Get();
    }

    ScopedRefPtr<T>& operator=(ScopedRefPtr<T>&& r) noexcept {
        ScopedRefPtr<T>(std::move(r)).Swap(*this);
        return *this;
    }

    template <typename U>
    ScopedRefPtr<T>& operator=(ScopedRefPtr<U>&& r) noexcept {
        ScopedRefPtr<T>(std::move(r)).Swap(*this);
        return *this;
    }

    void Swap(T** pp) noexcept {
        T* ptr = mPtr;
        mPtr = *pp;
        *pp = ptr;
    }

    void Swap(ScopedRefPtr<T>& r) noexcept { Swap(&r.mPtr); }

    ScopedRefPtr(ScopedRefPtr<T>&& r) noexcept : mPtr(r.Release()) {}

    template <typename U>
    ScopedRefPtr(ScopedRefPtr<U>&& r) noexcept : mPtr(r.Release()) {}

    ~ScopedRefPtr() {
        if (mPtr != nullptr) {
            mPtr->Release();
        }
    }

private:
    T* mPtr;
};

}  // namespace VKRT