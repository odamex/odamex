// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2012 by The Odamex Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//	Gui Configuration
//
// AUTHORS:
//	 Michael Wood (mwoodj at huntsvegas dot org)
//
//-----------------------------------------------------------------------------

#include "typedefs.h"

#ifndef _GUI_CONFIG_H
#define _GUI_CONFIG_H

/**
agOdalaunch namespace.

All code for the ag-odalaunch launcher is contained within the agOdalaunch
namespace.
*/
namespace agOdalaunch {

#ifdef _WIN32
const char PATH_DELIMITER = ';';
#else
const char PATH_DELIMITER = ':';
#endif

/**
Read and write configuration options.

This static class provides methods that can be used to set and retrieve
configuration options. The config file is automatically created and opened
by the Agar library.
*/
class GuiConfig
{
public:
	/**
	Load the configuration.

	Requests that Agar load the configuration settings.

	@return True if an error occurred, False if successful.
	*/
	static bool Load();

	/**
	Save the configuration.

	Requests that Agar save the current configuration settings.

	@return True if an error occurred, False if successful.
	*/
	static bool Save();

	/**
	Unset a configuration option.

	This method unsets a configuration option. The option is removed from
	the configuration file upon saving.

	@param option The name of the option to unset.
	*/
	static void Unset(const std::string &option);

	/**
	Query the existence of a configuration option.

	This method queries the agar configuration for the existence of the
	specified configuration option.

	@param option The name of the configuration option.
	@return True if the option is defined, False if it is not.
	*/
	static bool IsDefined(const std::string &option);

	/**
	Write a configuration option with a string value.

	This method writes an option to the configuration with a string value.

	@param option The name of the option to set.
	@param value A string value.
	@return True if an error occurred, False if successful.
	*/
	static bool Write(const std::string &option, const std::string &value);

	/**
	Write a configuration option with a boolean value.

	This method writes an option to the configuration with a boolean
	value.

	@param option The name of the option to set.
	@param value A boolean value.
	@return True if an error occurred, False if successful.
	*/
	static bool Write(const std::string &option, const bool &value);

	/**
	Write a configuration option with an 8-bit signed integer value.

	This method writes an option to the configuration with an 8-bit signed
	integer value.

	@param option The name of the option to set.
	@param value An 8-bit integer value.
	@return True if an error occurred, False if successful.
	*/
	static bool Write(const std::string &option, const int8_t &value);

	/**
	Write a configuration option with a 16-bit signed integer value.

	This method writes an option to the configuration with a 16-bit signed
	integer value.

	@param option The name of the option to set.
	@param value A 16-bit signed integer value.
	@return True if an error occurred, False if successful.
	*/
	static bool Write(const std::string &option, const int16_t &value);

	/**
	Write a configuration option with a 32-bit signed integer value.

	This method writes an option to the configuration with a 32-bit signed
	integer value.

	@param option The name of the option to set.
	@param value A 32-bit signed integer value.
	@return True if an error occurred, False if successful.
	*/
	static bool Write(const std::string &option, const int32_t &value);

	/**
	Write a configuration option with an 8-bit unsigned integer value.

	This method writes an option to the configuration with an 8-bit unsigned
	integer value.

	@param option The name of the option to set.
	@param value An 8-bit unsigned integer value.
	@return True if an error occurred, False if successful.
	*/
	static bool Write(const std::string &option, const uint8_t &value);

	/**
	Write a configuration option with a 16-bit unsigned integer value.

	This method writes an option to the configuration with a 16-bit unsigned
	integer value.

	@param option The name of the option to set.
	@param value A 16-bit unsigned integer value.
	@return True if an error occurred, False if successful.
	*/
	static bool Write(const std::string &option, const uint16_t &value);

	/**
	Write a configuration option with a 32-bit unsigned integer value.

	This method writes an option to the configuration with a 32-bit unsigned
	integer value.

	@param option The name of the option to set.
	@param value A string value.
	@return True if an error occurred, False if successful.
	*/
	static bool Write(const std::string &option, const uint32_t &value);

	/**
	Write a configuration option with a 32-bit floating point value.

	This method writes an option to the configuration with a 32-bit floating
	point value.

	@param option The name of the option to set.
	@param value A 32-bit floating point value.
	@return True if an error occurred, False if successful.
	*/
	static bool Write(const std::string &option, const float &value);

	/**
	Write a configuration option with a 64-bit floating point value.

	This method writes an option to the configuration with a 64-bit floating
	point value.

	@param option The name of the option to set.
	@param value A string value.
	@return True if an error occurred, False if successful.
	*/
	static bool Write(const std::string &option, const double &value);

	/**
	Read a configuration option with a string value.

	This method reads an option from the configuration with a string value.

	@param option The name of the option to read.
	@param value A string to read the value into.
	@return True if an error occurred, False if successful.
	*/
	static bool Read(const std::string &option, std::string &value);

	/**
	Read a configuration option with a boolean value.

	This method reads an option from the configuration with a boolean
	value.

	@param option The name of the option to read.
	@param value A boolean to read the value into.
	@return True if an error occurred, False if successful.
	*/
	static bool Read(const std::string &option, bool &value);

	/**
	Read a configuration option with an 8-bit signed integer value.

	This method reads an option from the configuration with an 8-bit signed
	integer value.

	@param option The name of the option to read.
	@param value An 8-bit signed integer to read the value into.
	@return True if an error occurred, False if successful.
	*/
	static bool Read(const std::string &option, int8_t &value);

	/**
	Read a configuration option with a 16-bit signed integer value.

	This method reads an option from the configuration with a 16-bit signed
	integer value.

	@param option The name of the option to read.
	@param value A 16-bit signed integer to read the value into.
	@return True if an error occurred, False if successful.
	*/
	static bool Read(const std::string &option, int16_t &value);

	/**
	Read a configuration option with a 32-bit signed integer value.

	This method reads an option from the configuration with a 32-bit signed
	integer value.

	@param option The name of the option to read.
	@param value A 32-bit signed integer to read the value into.
	@return True if an error occurred, False if successful.
	*/
	static bool Read(const std::string &option, int32_t &value);

	/**
	Read a configuration option with an 8-bit unsigned integer value.

	This method reads an option from the configuration with an 8-bit unsigned
	integer value.

	@param option The name of the option to read.
	@param value An 8-bit unsigned integer to read the value into.
	@return True if an error occurred, False if successful.
	*/
	static bool Read(const std::string &option, uint8_t &value);

	/**
	Read a configuration option with a 16-bit unsigned integer value.

	This method reads an option from the configuration with a 16-bit unsigned
	integer value.

	@param option The name of the option to read.
	@param value A 16-bit unsigned integer to read the value into.
	@return True if an error occurred, False if successful.
	*/
	static bool Read(const std::string &option, uint16_t &value);

	/**
	Read a configuration option with a 32-bit unsigned integer value.

	This method reads an option from the configuration with a 32-bit unsigned
	integer value.

	@param option The name of the option to read.
	@param value A 32-bit unsigned integer to read the value into.
	@return True if an error occurred, False if successful.
	*/
	static bool Read(const std::string &option, uint32_t &value);

	/**
	Read a configuration option with a 32-bit floating point value.

	This method reads an option from the configuration with a 32-bit floating
	poing value.

	@param option The name of the option to read.
	@param value A 32-bit floating point to read the value into.
	@return True if an error occurred, False if successful.
	*/
	static bool Read(const std::string &option, float &value);

	/**
	Read a configuration option with a 64-bit floating point value.

	This method reads an option from the configuration with a 64-bit floating
	point value.

	@param option The name of the option to read.
	@param value A 64-bit floating point to read the value into.
	@return True if an error occurred, False if successful.
	*/
	static bool Read(const std::string &option, double &value);

private:
	// Static class
	GuiConfig();
	GuiConfig(const GuiConfig&);
	GuiConfig& operator=(const GuiConfig&);
};

} // namespace

#endif
