#include "tinylog.hpp"

namespace tinylog
{

//////////////////////////////////////////////////////////////////////////////
//
// registry_impl 注册管理
//
//////////////////////////////////////////////////////////////////////////////

namespace detail
{

class registry_impl
{
public:
#if defined(TINYLOG_NO_REGISTRY_MUTEX)
    using mutex_type = detail::null_mutex>;
#else
    using mutex_type = std::mutex;
#endif

#if defined(TINYLOG_WINDOWS_API)
    using char_type     = wchar_t;
    using extern_type   = char;
#else
    using char_type     = char;
    using extern_type   = wchar_t;
#endif
    using string_t      = std::basic_string<char_type>;
    using xstring_t     = std::basic_string<extern_type>;
    using logger_t      = logger;
    using logger_ptr    = std::shared_ptr<logger_t>;

public:
    static registry_impl& instance()
    {
        static bool f = false;
        if(!f)
        {
            // This code *must* be called before main() starts,
            // and when only one thread is executing.
            f = true;
            s_inst_ = std::make_shared<registry_impl>();
        }

        // The following line does nothing else than force the instantiation
        // of singleton<T>::create_object, whose constructor is
        // called before main() begins.
        s_create_object_.do_nothing();
        return *s_inst_;
    }

public:
    registry_impl() = default;
    registry_impl(registry_impl const&) = delete;
    registry_impl& operator=(registry_impl const&) = delete;

    // 日志级别:
    //     为创建日志记录器时设置默认过滤级别
    void set_level(level lvl)
    {
        std::lock_guard<mutex_type> lock(mtx_);
        for (auto& l : loggers_)
        {
            l.second->set_level(lvl);
        }
        lvl_ = lvl;
    }

    // 添加日志记录器
    //      失败将抛出异常
    logger_ptr create_logger(string_t const& logger_name)
    {
        std::lock_guard<mutex_type> lock(mtx_);
        throw_if_exists(logger_name);
        auto p = std::make_shared<logger_t>(logger_name);
        p->set_level(lvl_);
        loggers_[logger_name] = p;
        return p;
    }
    logger_ptr create_logger(xstring_t const& logger_name)
    {
        string_t name;
        string_traits<>::convert(name, logger_name);
        return create_logger(name);
    }
    logger_ptr create_logger()
    {
#if defined(TINYLOG_WINDOWS_API)
        return create_logger(TINYLOG_DEFAULTW);
#else
        return create_logger(TINYLOG_DEFAULT);
#endif
    }

    logger_ptr add_logger(logger_ptr inst)
    {
        assert(inst && "logger instance point must exists");
        auto const name = cvt(inst->name());
        std::lock_guard<mutex_type> lock(mtx_);
        throw_if_exists(name);
        inst->set_level(lvl_);
        loggers_[name] = inst;
        return inst;
    }

    // 获取日志记录器
    //      失败返回空指针
    logger_ptr get_logger(string_t const& logger_name) const
    {
        std::lock_guard<mutex_type> lock(mtx_);
        auto found = loggers_.find(logger_name);
        return found == loggers_.cend() ? nullptr : found->second;
    }
    logger_ptr get_logger(xstring_t const& logger_name) const
    {
        string_t name;
        string_traits<>::convert(name, logger_name);
        return get_logger(name);
    }
    logger_ptr get_logger() const
    {
#if defined(TINYLOG_WINDOWS_API)
        return get_logger(TINYLOG_DEFAULTW);
#else
        return get_logger(TINYLOG_DEFAULT);
#endif
    }

    void erase_logger(string_t const& logger_name)
    {
        std::lock_guard<mutex_type> lock(mtx_);
        loggers_.erase(logger_name);
    }
    void erase_logger(xstring_t const& logger_name)
    {
        string_t name;
        string_traits<>::convert(name, logger_name);
        return erase_logger(name);
    }
    void erase_logger()
    {
#if defined(TINYLOG_WINDOWS_API)
        return erase_logger(TINYLOG_DEFAULTW);
#else
        return erase_logger(TINYLOG_DEFAULT);
#endif
    }

    void erase_all_logger()
    {
        std::lock_guard<mutex_type> lock(mtx_);
        loggers_.clear();
    }

private:
    static string_t cvt(string_t const& xs)
    {
        return xs;
    }
    static string_t cvt(xstring_t const& xs)
    {
        string_t s;
        string_traits<>::convert(s, xs);
        return s;
    }

    void throw_if_exists(string_t const& logger_name) const
    {
        if (loggers_.find(logger_name) != loggers_.cend())
        {
            std::string name;
            string_traits<>::convert(name, logger_name);
            name = "logger with name \'" + name + "\' already exists";
            throw std::system_error(std::make_error_code(std::errc::file_exists)
                                    , name);
        }
    }

private:
    struct object_creator
    {
        object_creator()
        {
            // This constructor does nothing more than ensure that instance()
            // is called before main() begins, thus creating the static
            // T object before multithreading race issues can come up.
            detail::registry_impl::instance();
        }
        inline void do_nothing() const
        {
        }
    };

private:
    static std::shared_ptr<registry_impl> s_inst_;
    static object_creator s_create_object_;

private:
    mutable mutex_type mtx_;
    level lvl_ = level::trace;
    std::unordered_map<string_t, logger_ptr> loggers_;
};

std::shared_ptr<registry_impl> registry_impl::s_inst_;
registry_impl::object_creator registry_impl::s_create_object_;

} // namespace detail

//////////////////////////////////////////////////////////////////////////////
//
// registry 实现
//
//////////////////////////////////////////////////////////////////////////////

void registry::set_level(level lvl)
{
    return detail::registry_impl::instance().set_level(lvl);
}

registry::logger_ptr registry::create_logger(string_t const& logger_name)
{
    return detail::registry_impl::instance().create_logger(logger_name);
}

registry::logger_ptr registry::create_logger(xstring_t const& logger_name)
{
    return detail::registry_impl::instance().create_logger(logger_name);
}

registry::logger_ptr registry::create_logger()
{
    return detail::registry_impl::instance().create_logger();
}

registry::logger_ptr registry::add_logger(logger_ptr inst)
{
    return detail::registry_impl::instance().add_logger(inst);
}

registry::logger_ptr registry::get_logger(string_t const& logger_name)
{
    return detail::registry_impl::instance().get_logger(logger_name);
}

registry::logger_ptr registry::get_logger(xstring_t const& logger_name)
{
    return detail::registry_impl::instance().get_logger(logger_name);
}

registry::logger_ptr registry::get_logger()
{
    return detail::registry_impl::instance().get_logger();
}

void registry::erase_logger(string_t const& logger_name)
{
    return detail::registry_impl::instance().erase_logger(logger_name);
}

void registry::erase_logger(xstring_t const& logger_name)
{
    return detail::registry_impl::instance().erase_logger(logger_name);
}

void registry::erase_logger()
{
    return detail::registry_impl::instance().erase_logger();
}

void registry::erase_all_logger()
{
    return detail::registry_impl::instance().erase_all_logger();
}

} // namespace tinylog
