/* HB Core - Homebax Studios. Copyright (c) 2026 Ondrej Jurasik / Homebax Studios. */
#ifndef HBCORE_HB_EXPORT_H
#define HBCORE_HB_EXPORT_H

#if defined(HBCORE_STATIC)
#  define HB_API
#elif defined(_WIN32)
#  if defined(HBCORE_BUILDING)
#    define HB_API __declspec(dllexport)
#  else
#    define HB_API __declspec(dllimport)
#  endif
#else
#  define HB_API __attribute__((visibility("default")))
#endif

#endif
