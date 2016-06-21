#pragma once

template<typename HandleType, typename InvalidValueType, InvalidValueType ConstInvalidValue, typename ReleaseResource, ReleaseResource ReleaseFunc>
class resource_handle
{
    const HandleType InvalidValue = (HandleType)ConstInvalidValue;

    HandleType m_devInfo;

public:
    resource_handle() : m_devInfo(InvalidValue) { }
    resource_handle(HandleType handle) : m_devInfo(handle) { }

    resource_handle(resource_handle const & other) = delete;
    resource_handle & operator=(resource_handle const & other) = delete;

    resource_handle(resource_handle&& other)
    {
        m_devInfo = other.release();
    }

    resource_handle & operator=(resource_handle&& other)
    {
        reset(other.release());

        return *this;
    }

    ~resource_handle()
    {
        reset(InvalidValue);
    }

    operator bool() const { return m_devInfo != InvalidValue; }

    void reset(HandleType devInfo)
    {
        if (*this)
        {
            ReleaseFunc(m_devInfo);
        }

        m_devInfo = devInfo;
    }

    HandleType release()
    {
        auto devInfo = m_devInfo;
        m_devInfo = InvalidValue;
        return devInfo;
    }

    HandleType get() const
    {
        return m_devInfo;
    }
};