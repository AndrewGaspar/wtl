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
    class result_t
    {
        ResultType m_result;
        std::uint8_t m_data[sizeof(Value)];

        result_t()
        {

        }

    protected:
        void reset()
        {
            if (*this)
            {
                auto & value = get();
                value.~Value();
            }
        }

        template<typename RValue>
        void init(RValue&& v)
        {
            new (m_data) Value(std::forward<RValue>(v));
        }

        template<typename RValue>
        void reset(RValue&& v)
        {
            reset();

            init(std::forward<RValue>(v));
        }

        void set_result(ResultType result)
        {
            m_result = result;
        }
    public:

        // intentionally implicit
        result_t(ResultType error)
        {
            if (!IsFailure(error))
            {
                throw std::runtime_error("Cannot implicitly construct a result from a non-failure error code");
            }

            m_result = error;
        }

        result_t(result_t&& other)
        {
            m_result = other.m_result;

            if (other)
            {
                new (m_data) Value(std::move(other.get()));
            }
        }

        ~result_t()
        {
            reset();
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
        static result_t success(_Val&& value, ResultType res = Success)
        {
            ASSERT(!IsFailure(res));

            auto r = result_t();
            r.set_result(res);
            r.init(std::forward<_Val>(value));
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

#ifdef _ERRHANDLING_H_

    namespace details
    {
        constexpr bool IsWin32Error(DWORD err)
        {
            return ERROR_SUCCESS != err;
        }
    }

    template<typename T>
    using win32_err_t = result_t<T, DWORD, ERROR_SUCCESS, details::IsWin32Error>;

    using win32_err = win32_err_t<void>;

#endif

#ifdef _HRESULT_DEFINED

    namespace details
    {
        constexpr bool IsHresultFail(HRESULT hr)
        {
            return FAILED(hr);
        }
    }

    template<typename T>
    struct hresult_t : public result_t<T, HRESULT, S_OK, details::IsHresultFail>
    {
        hresult_t(HRESULT hr) : result_t(hr) { }

        template<typename _Val>
        static hresult_t success(_Val&& value, HRESULT res = S_OK)
        {
            if (FAILED(res))
            {
                throw std::runtime_error("Cannot construct hresult_t with an E_ failure code")
            }

            auto r = hresult_t();
            r.set_result(res);
            r.init(std::forward<_Val>(v));
            return r;
        }

#ifdef _ERRHANDLING_H_
        hresult_t(win32_err_t<T>&& win32Err) : this()
        {
            set_result(HRESULT_FROM_WIN32(win32Err.get_result()));

            if (win32Err)
            {
                reset(std::move(win32Err.get()));
            }
        }
#endif
    };

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
