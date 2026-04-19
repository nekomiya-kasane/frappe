#ifndef FRAPPE_EXPORTS_HPP
#define FRAPPE_EXPORTS_HPP

#ifdef _WIN32
#  ifdef FRAPPE_BUILD_INTERNAL
#    define FRAPPE_API __declspec(dllexport)
#  else
#    define FRAPPE_API __declspec(dllimport)
#  endif
#else
#  define FRAPPE_API __attribute__((visibility("default")))
#endif

#endif // FRAPPE_EXPORTS_HPP
