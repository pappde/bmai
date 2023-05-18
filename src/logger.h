///////////////////////////////////////////////////////////////////////////////////////////
// logger.h
// Copyright (c) 2001-2023 Denis Papp. All rights reserved.
// denis@accessdenied.net
// https://github.com/hamstercrack/bmai
// 
// DESC: logging class
//
// REVISION HISTORY:
// drp030321 - partial split out to individual headers
///////////////////////////////////////////////////////////////////////////////////////////

#pragma once

class BMC_Logger
{
public:
	BMC_Logger();

	// methods
	void			Log(BME_DEBUG _cat, char *_fmt, ...);

	// mutators
	void			SetLogging(BME_DEBUG _cat, bool _log) { m_logging[_cat] = _log; }
	bool			SetLogging(const char *_catname, bool _log);

	// accessors
	bool			IsLogging(BME_DEBUG _cat) { return m_logging[_cat]; }

private:
	bool			m_logging[BME_DEBUG_MAX];
};