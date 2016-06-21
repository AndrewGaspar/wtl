#pragma once

#undef max

#define REQUIRE_SEMICOLON (true)

#ifdef _DEBUG
#define ASSERT(_expr) {\
    if (!_expr) \
    { \
    std::terminate(); \
    } \
} REQUIRE_SEMICOLON
#else
#define ASSERT(_expr)
#endif

#include <type_traits>
#include <algorithm>
#include <stdexcept>

namespace wtl 
{
    template<typename ResultType>
    class result_exception : public std::runtime_error
    {
        ResultType errorCode;
    public:
        result_exception(ResultType errorCode) : errorCode(errorCode), std::runtime_error("Error occurred") { }

        ResultType error() const
        {
            return errorCode;
        }
    };

    template<typename Value, typename ResultType, ResultType Success, bool IsFailure(ResultType)>
    class result
    {
        ResultType m_result;
        std::uint8_t m_data[sizeof(Value)];

        result()
        {

        }

    public:

        // intentionally implicit

        result(ResultType error)
        {
            ASSERT(IsFailure(error));

            m_result = error;
        }

        result(result&& other)
        {
            m_result = other.m_result;

            if (other)
            {
                new (m_data) Value(std::move(other.get()));
            }
        }

        ~result()
        {
            if (*this)
            {
                auto & value = get();
                value.~Value();
            }
        }

        operator ResultType() const
        {
            return get_result();
        }

        ResultType get_result() const
        {
            return m_result;
        }

        Value & get()
        {
            ASSERT(*this);

            return *reinterpret_cast<Value*>(m_data);
        }

        Value const & get() const
        {
            ASSERT(*this);

            return *reinterpret_cast<Value const *>(m_data);
        }

        operator bool() const
        {
            return !IsFailure(get_result());
        }

        template<typename _Val>
        static result success(_Val&& value, ResultType res = Success)
        {
            ASSERT(!IsFailure(res));

            auto r = result();
            r.m_result = res;
            new (r.m_data) Value(std::forward<_Val>(value));
            return r;
        }

        void throw_if_failed()
        {
            if (!*this)
            {
                throw result_exception<ResultType>(get_result());
            }
        }
    };

    template<typename Value, typename ResultType, ResultType Success, bool IsFailure(ResultType)>
    using result_t = typename result<Value, ResultType, Success, IsFailure>;

#ifdef _HRESULT_DEFINED

    namespace details
    {
        constexpr bool IsHresultFail(HRESULT hr)
        {
            return FAILED(hr);
        }
    }

    template<typename T>
    using hresult_t = result_t<T, HRESULT, S_OK, details::IsHresultFail>;

    using hresult = hresult_t<void>;

#endif

}

#define CONCAT_TOKENS3(a,b,c)      a##b##c
#define EXPAND_THEN_CONCAT3(a,b,c) CONCAT_TOKENS3(a,b,c)
#define _WTL_ID_()                 EXPAND_THEN_CONCAT3(__wtl_,__LINE__,_result)

#define RETURN_OR_UNWRAP(_resultName, _resultExpression) \
    auto _WTL_ID_() = _resultExpression; \
    if (!_WTL_ID_()) \
    { \
        return _WTL_ID_().get_result(); \
    } \
    auto _resultName = std::move(_WTL_ID_().get())

#define RETURN_IF_HR_FAILED(_hrExpression) { \
    auto _wtl_result = _hrExpression; \
    if (FAILED(_wtl_result)) \
    { \
        return _wtl_result; \
    } \
} REQUIRE_SEMICOLON

#define RETURN_IF_LAST_ERROR() RETURN_IF_HR_FAILED(HRESULT_FROM_WIN32(GetLastError()))
