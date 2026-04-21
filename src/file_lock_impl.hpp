#ifndef FRAPPE_FILE_LOCK_IMPL_HPP
#define FRAPPE_FILE_LOCK_IMPL_HPP

#include "frappe/file_lock.hpp"

namespace frappe {

    class file_lock::impl {
      public:
        path _file_path;
        lock_options _options;
        bool _owns_lock = false;

#ifdef _WIN32
        void *_handle = nullptr;
#else
        int _fd = -1;
#endif

        impl(const path &p, lock_options opts) : _file_path(p), _options(opts) {}

        ~impl();

        void_result lock();
        void_result try_lock();
        void_result try_lock_for(std::chrono::milliseconds timeout);
        void unlock();
    };

} // namespace frappe

#endif // FRAPPE_FILE_LOCK_IMPL_HPP
