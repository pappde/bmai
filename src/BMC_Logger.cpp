///////////////////////////////////////////////////////////////////////////////////////////
// BMC_Logger.cpp
// Copyright (c) 2001-2024 Denis Papp. All rights reserved.
// denis@accessdenied.net
// https://github.com/pappde/bmai
// 
// DESC: logging class
//
// REVISION HISTORY:
// dbl100524 - broke this logic out into its own class file
///////////////////////////////////////////////////////////////////////////////////////////

#include "BMC_Logger.h"

#include <algorithm>
#include <cstring>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <string>


// BME_DEBUG setting names
const char *g_debug_name[BME_DEBUG_MAX] =
{
  "ALWAYS",
  "WARNING",
  "PARSER",
  "SIMULATION",
  "ROUND",
  "GAME",
  "QAI",
  "BMAI",
};

//global
BMC_Logger	g_logger;

BMC_Logger::BMC_Logger()
{
  for (INT i = 0; i<BME_DEBUG_MAX; i++)
    m_logging[i] = true;
}

void BMC_Logger::Log(BME_DEBUG _cat, const char *_fmt, ... )
{
  if (!m_logging[_cat])
    return;

  va_list	ap;
  va_start(ap, _fmt),
  vprintf( _fmt, ap );
  va_end(ap);
}

bool BMC_Logger::SetLogging(const char *_catname, bool _log)
{
  // case-insensitive compares are not easy to come by in a cross-platform way
  // so lets uppercase the input and do a normal std::strcmp below
  std::string _cn(_catname);
  std::transform(_cn.begin(), _cn.end(), _cn.begin(), ::toupper);

  INT i;
  for (i=0; i<BME_DEBUG_MAX; i++)
  {
    if (!std::strcmp(_catname, g_debug_name[i]))
    {
      SetLogging((BME_DEBUG)i, _log);
      Log(BME_DEBUG_ALWAYS, "Debug %s set to %d\n", _catname, _log);
      return true;
    }
  }

  BMF_Error("Could not find debug category: %s\n", _catname);
  return false;
}

///////////////////////////////////////////////////////////////////////////////////////////
// logging methods
///////////////////////////////////////////////////////////////////////////////////////////

void BMF_Error( const char *_fmt, ... )
{
  va_list	ap;
  va_start(ap, _fmt),
  vprintf( _fmt, ap );
  va_end(ap);

  exit(1);
}

void BMF_Log(BME_DEBUG _cat, const char *_fmt, ... )
{
  if (!g_logger.IsLogging(_cat))
    return;

  va_list	ap;
  va_start(ap, _fmt),
  vprintf( _fmt, ap );
  va_end(ap);
}
