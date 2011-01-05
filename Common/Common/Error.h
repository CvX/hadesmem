/*
This file is part of HadesMem.
Copyright © 2010 Cypherjb (aka Chazwazza, aka Cypher). 
<http://www.cypherjb.com/> <cypher.jb@gmail.com>

HadesMem is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

HadesMem is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with HadesMem.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

// C++ Standard Library
#include <string>
#include <stdexcept>
#include <cerrno>

// Boost
#ifdef _MSC_VER
#pragma warning(push, 1)
#endif // #ifdef _MSC_VER
#include <boost/exception/all.hpp>
#include <boost/system/error_code.hpp>
#ifdef _MSC_VER
#pragma warning(pop)
#endif // #ifdef _MSC_VER

// Posh
#include <posh.h>

// System 
#if defined(POSH_OS_WIN32)
# include <Windows.h>
#endif

namespace Hades
{
  // Error info (function name)
  typedef boost::error_info<struct TagErrorFunc, std::string> ErrorFunction;
  // Error info (error string)
  typedef boost::error_info<struct TagErrorString, std::string> ErrorString;
  // Error info (System error code)
  typedef boost::error_info<struct TagSysErrorCode,
    boost::system::error_code> ErrorCodeSys;

  // Create a error code
  inline boost::system::error_code MakeErrorCodeSys()
  {
    using bs = boost::system;

  #if defined(POSH_OS_WIN32)
    return bs::error_code(GetLastError(), bs::system_category);
  #elif defined(POSH_OS_LINUX) || defined(POSH_OS_UNIX)
    return bs::error_code(errno, bs::system_category);
  #endif
  }

  // Base exception class
  class HadesError : public virtual std::exception, 
    public virtual boost::exception
  { };

}
