///////////////////////////////////////////////////////////////////////////////////////////
// BMC_Logger.h
// Copyright (c) 2001-2024 Denis Papp. All rights reserved.
// denis@accessdenied.net
// https://github.com/pappde/bmai
// 
// DESC: logging class
//
// REVISION HISTORY:
// drp030321 - partial split out to individual headers
// dbl100524 - further split out of individual headers
///////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <cassert>
#include "bmai_lib.h"


class BMC_Logger
{
public:
	BMC_Logger();

	// methods
	void			Log(BME_DEBUG _cat, const char *_fmt, ...);

	// mutators
	void			SetLogging(BME_DEBUG _cat, bool _log) { m_logging[_cat] = _log; }
	bool			SetLogging(const char *_catname, bool _log);

	// accessors
	bool			IsLogging(BME_DEBUG _cat) { return m_logging[_cat]; }

private:
	bool			m_logging[BME_DEBUG_MAX];
};


// To address a cyclical dependency the global logger can live here for now
// TODO address & understand g_logger.Log() vs BMF_Log()
extern BMC_Logger	g_logger;

// Utility functions

void BMF_Error( const char *_fmt, ... );
void BMF_Log(BME_DEBUG _cat, const char *_fmt, ... );

// assert

#define BM_ASSERT	assert
#define BM_ERROR(check)	{ if (!(check)) { BMF_Error(#check); } }
