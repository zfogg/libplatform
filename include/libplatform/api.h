/**
 * @file libplatform/api.h
 * @brief DLL import/export annotations for libplatform.
 */
#pragma once

#if defined(_WIN32)
#  if defined(BUILDING_LIBPLATFORM_DLL)
#    define LIBPLATFORM_API __declspec(dllexport)
#  elif defined(BUILDING_STATIC_LIB)
#    define LIBPLATFORM_API
#  else
#    define LIBPLATFORM_API __declspec(dllimport)
#  endif
#elif defined(__GNUC__) || defined(__clang__)
#  define LIBPLATFORM_API __attribute__((visibility("default")))
#else
#  define LIBPLATFORM_API
#endif
