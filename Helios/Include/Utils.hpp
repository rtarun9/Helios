#pragma once

inline void fatalError(const std::string_view message,
                       const std::source_location sourceLocation = std::source_location::current())
{
    const std::string errorMessage =
        std::format("[FATAL ERROR] :: {}. Source Location data : File Name -> {}, Function Name -> "
                    "{}, Line Number -> {}, Column -> {}.\n",
                    message, sourceLocation.file_name(), sourceLocation.function_name(), sourceLocation.line(),
                    sourceLocation.column());

    throw std::runtime_error(errorMessage);
}

inline void log(const std::string_view message)
{
    std::cout << "[LOG] :: " << message << '\n';
}

inline void log(const std::wstring_view message)
{
    std::wcout << L"[LOG] :: " << message << '\n';
}

inline void throwIfFailed(const HRESULT hr, const std::source_location sourceLocation = std::source_location::current())
{
    if (FAILED(hr))
    {
        fatalError("HRESULT failed!", sourceLocation);
    }
}

inline std::wstring stringToWString(const std::string_view inputString)
{
    std::wstring result{};
    const std::string input{inputString};

    const int32_t length = ::MultiByteToWideChar(CP_UTF8, 0, input.c_str(), -1, NULL, 0);
    if (length > 0)
    {
        result.resize(size_t(length) - 1);
        MultiByteToWideChar(CP_UTF8, 0, input.c_str(), -1, result.data(), length);
    }

    return std::move(result);
}

inline std::string wStringToString(const std::wstring_view inputWString)
{
    std::string result{};
    const std::wstring input{inputWString};

    const int32_t length = ::WideCharToMultiByte(CP_UTF8, 0, input.c_str(), -1, NULL, 0, NULL, NULL);
    if (length > 0)
    {
        result.resize(size_t(length) - 1);
        WideCharToMultiByte(CP_UTF8, 0, input.c_str(), -1, result.data(), length, NULL, NULL);
    }

    return std::move(result);
}

template <typename T>
static inline constexpr typename std::underlying_type<T>::type enumClassValue(const T& value)
{
    return static_cast<std::underlying_type<T>::type>(value);
}
