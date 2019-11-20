#include "tinylog/tinylog.hpp"


namespace tinylog
{
namespace detail
{

template <class mutexT>
void registry_impl<mutexT>::ensure_impl_in_shared_lib() const
{
    // Nothing to do.
}

// Explicit instantiation.
// template class registry_impl<>;
template class registry_impl<std::mutex>;
template class registry_impl<::tinylog::detail::null_mutex>;

} // namespace detail
} // namespace tinylog
