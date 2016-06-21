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

    template<typename ResultType, ResultType Success, bool IsFailure(ResultType)>
    class result
    {
        ResultType m_result;

    protected:
        void set_result(ResultType result)
        {
            m_result = result;
        }

    public:
        result() : m_result(Success) { }

        result(ResultType error) : m_result(error) { }

        ResultType get_result() const
        {
            return m_result;
        }

        operator bool() const
        {
            return !IsFailure(get_result());
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
    class result_t : public result<ResultType, Success, IsFailure>
    {
        std::uint8_t m_data[sizeof(Value)];

    protected:
        result_t() : result()
        {

        }

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
    public:

        using value_type = Value;

        // intentionally implicit
        result_t(ResultType error) : result(error)
        {
            if (!IsFailure(error))
            {
                throw std::runtime_error("Cannot implicitly construct a result from a non-failure error code");
            }
        }

        result_t(result_t&& other) : result(other.get_result())
        {
            if (other)
            {
                new (m_data) Value(std::move(other.get()));
            }
        }

        ~result_t()
        {
            reset();
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

        template<typename _Val>
        static result_t success(_Val&& value, ResultType res = Success)
        {
            ASSERT(!IsFailure(res));

            auto r = result_t();
            r.set_result(res);
            r.init(std::forward<_Val>(value));
            return r;
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

    using win32_err = result<DWORD, ERROR_SUCCESS, details::IsWin32Error>;

#endif

#ifdef _CFGMGR32_H_
    namespace details
    {
        constexpr bool IsConfigretFail(CONFIGRET cr)
        {
            return cr != CR_SUCCESS;
        }
    }

    template<typename T>
    using configret_t = result_t<T, CONFIGRET, CR_SUCCESS, details::IsConfigretFail>;

    using configret = result<CONFIGRET, CR_SUCCESS, details::IsConfigretFail>;

#endif

#ifdef _HRESULT_DEFINED

    namespace details
    {
        constexpr bool IsHresultFail(HRESULT hr)
        {
            return FAILED(hr);
        }
    }

    using hresult = result<HRESULT, S_OK, details::IsHresultFail>;

    template<typename T>
    class hresult_t : public result_t<T, HRESULT, S_OK, details::IsHresultFail>
    {
        hresult_t() : result_t() { }

    public:

        hresult_t(HRESULT hr) : result_t(hr) { }

        operator hresult()
        {
            return hresult(get_result());
        }

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
        static hresult_t from_win32(win32_err_t<T> const & win32Err)
        {
            auto r = hresult_t();
            r.set_result(HRESULT_FROM_WIN32(win32Err.get_result()));
            if (win32Err) r.init(win32Err.get());
            return r;
        }

        static hresult_t from_win32(win32_err_t<T>&& win32Err)
        {
            auto r = hresult_t();
            r.set_result(HRESULT_FROM_WIN32(win32Err.get_result()));
            if (win32Err) r.init(std::move(win32Err.get()));
            return r;
        }

#ifdef _CFGMGR32_H_
        static hresult_t from_configret(configret_t<T> const & configretErr)
        {
            auto r = hresult_t();
            r.set_result(
                HRESULT_FROM_WIN32(
                    CM_MapCrToWin32Err(configretErr.get_result(), ERROR_INVALID_FUNCTION)));
            if (configretErr) r.init(configretErr.get());
            return r;
        }

        static hresult_t from_configret(configret_t<T>&& configretErr)
        {
            auto r = hresult_t();
            r.set_result(
                HRESULT_FROM_WIN32(
                    CM_MapCrToWin32Err(configretErr.get_result(), ERROR_INVALID_FUNCTION)));
            if (configretErr) r.init(std::move(configretErr.get()));
            return r;
        }
#endif

#endif
    };


#ifdef _ERRHANDLING_H_
    template<
        typename Win32ErrT, 
        typename T = Win32ErrT::value_type,
        typename = std::enable_if_t<std::is_same<std::decay_t<Win32ErrT>, win32_err_t<T>>::value, void>>
    static hresult_t<T> hresult_from_win32(Win32ErrT&& errT)
    {
        return hresult_t<T>::from_win32(std::forward<Win32ErrT>(errT));
    }

#ifdef _CFGMGR32_H_
    template<
        typename ConfigRetT,
        typename T = ConfigRetT::value_type,
        typename = std::enable_if_t<std::is_same<std::decay_t<ConfigRetT>, configret_t<T>>::value, void>>
    static hresult_t<T> hresult_from_configret(ConfigRetT&& errT)
    {
        return hresult_t<T>::from_configret(std::forward<ConfigRetT>(errT));
    }
#endif

#endif

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
