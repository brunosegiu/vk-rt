#pragma once

#include <tuple>

namespace VKRT {

enum class Result {
    Success,
    InvalidInputFormatError,
    InvalidOutputFormatError,
    InvalidInputResolutionError,
    InvalidOutputResolutionError,
    DriverNotFoundError,
    InvalidDeviceError,
    NoSuitableDeviceError,
    UnknownError
};

template <typename T>
struct ResultValue {
    Result result;
    T value;

    operator std::tuple<Result&, T&>() { return std::tuple<Result&, T&>(result, value); }
};
}  // namespace VKRT