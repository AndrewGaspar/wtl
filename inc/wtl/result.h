
#ifndef _RESULT_H_
#define _RESULT_H_

#undef max

#include <type_traits>
#include <algorithm>
#include <stdexcept>

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

        result_t & operator=(result_t const & rhs) = delete;

        result_t & operator=(result_t&& rhs)
        {
            set_result(rhs.get_result());

            if (*this && rhs)
            {
                get() = std::move(rhs.get());
                return *this;
            }

            if (*this)
            {
                reset();
                return *this;
            }

            if (rhs)
            {
                reset(std::move(rhs.get()));
                return *this;
            }

            return *this;
        }

        ~result_t()
        {
            reset();
        }

        Value & get() &
        {
            ASSERT(*this);

            return *reinterpret_cast<Value*>(m_data);
        }

        Value const & get() const &
        {
            ASSERT(*this);

            return *reinterpret_cast<Value const *>(m_data);
        }

        Value&& get() &&
        {
            ASSERT(*this);

            return std::move(*reinterpret_cast<Value*>(m_data));
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

#endif _RESULT_H_

namespace wtl
{

#ifdef _ERRHANDLING_H_

#ifndef _WIN32_RESULT_
#define _WIN32_RESULT_

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

#endif

#ifdef _CFGMGR32_H_

#ifndef _CFGMGR32_RESULT_
#define _CFGMGR32_RESULT_

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

#endif //_CFGMGR32_RESULT_
#endif //_CFGMGR32_H_

#ifdef _HRESULT_DEFINED

#ifndef _HRESULT_RESULT_
#define _HRESULT_RESULT_

    namespace details
    {
        constexpr bool IsHresultFail(HRESULT hr)
        {
            return FAILED(hr);
        }
    }

    using hresult = result<HRESULT, S_OK, details::IsHresultFail>;

    template<typename T>
    using hresult_t = result_t<T, HRESULT, S_OK, details::IsHresultFail>;

#ifdef _ERRHANDLING_H_
    template<
        typename Win32ErrT, 
        typename T = Win32ErrT::value_type,
        typename = std::enable_if_t<std::is_same<std::decay_t<Win32ErrT>, win32_err_t<T>>::value, void>>
    static hresult_t<T> hresult_from_win32_t(Win32ErrT&& errT)
    {
        if (!errT)
        {
            return HRESULT_FROM_WIN32(errT.get_result());
        }

        return hresult_t<T>::success(std::forward<Win32ErrT>(errT).get());
    }

#ifdef _CFGMGR32_H_
    template<
        typename ConfigRetT,
        typename T = ConfigRetT::value_type,
        typename = std::enable_if_t<std::is_same<std::decay_t<ConfigRetT>, configret_t<T>>::value, void>>
    static hresult_t<T> hresult_from_configret_t(ConfigRetT&& errT)
    {
        if (!errT)
        {
            return HRESULT_FROM_WIN32(CM_MapCrToWin32Err(errT.get_result(), ERROR_INVALID_FUNCTION));
        }

        return hresult_t<T>::success(std::forward<ConfigRetT>(errT).get());
    }
#endif // _CFGMGR32_H_
#endif // _ERRHANDLING_H_

#endif //_HRESULT_RESULT_
#endif // _HRESULT_DEFINED

}

#ifdef _HRESULT_DEFINED

#define RETURN_IF_HR_FAILED(_hrExpression) { \
    auto _wtl_result = _hrExpression; \
    if (FAILED(_wtl_result)) \
    { \
        return _wtl_result; \
    } \
} REQUIRE_SEMICOLON

#ifdef _ERRHANDLING_H_

#define RETURN_IF_LAST_ERROR() RETURN_IF_HR_FAILED(HRESULT_FROM_WIN32(GetLastError()))

#endif 
#endif
