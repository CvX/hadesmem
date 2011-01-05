/*
This file is part of HadesMem.
Copyright © 2010 RaptorFactor (aka Cypherjb, Cypher, Chazwazza). 
<http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

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

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
	#pragma once
#endif

#ifndef __HADES_MEMORY_TYPES__
#define __HADES_MEMORY_TYPES__

// C++ Standard Library
#include <string>
#include <cstdint>

// Posh
#include "posh.h"

// System
#if defined(POSH_OS_WIN32)
# include <Windows.h>
#elif defined(POSH_OS_LINUX)
# include <sys/types.h>
#else
# error [HadesMem] Operating system not supported
#endif

namespace Hades
{
  namespace Memory
  {
    namespace Types
    {
      // Declare signed integer types
      typedef std::int8_t   Int8;
      typedef std::int16_t  Int16;
      typedef std::int32_t  Int32;
      typedef std::int64_t  Int64;
      typedef std::intptr_t IntPtr;

      // Declare signed integer types
      typedef std::uint8_t   UInt8;
      typedef std::uint16_t  UInt16;
      typedef std::uint32_t  UInt32;
      typedef std::uint64_t  UInt64;
      typedef std::uintptr_t UIntPtr;

      // Declare char & string types
      typedef char      Char8;
      typedef char16_t  Char16;
      typedef char32_t  Char32;
      
      typedef std::basic_string<Char8>  String8;
      typedef std::basic_string<Char16> String16;
      typedef std::basic_string<Char32> String32;

      typedef char          CharA;
      typedef wchar_t       CharW;
      typedef std::string   StringA;
      typedef std::wstring  StringW;

    #if defined(UNICODE) || defined(_UNICODE)
      typedef CharW   TChar;
      typedef StringW TString;
    #else
      typedef CharA   TChar;
      typedef StringA TString;
    #endif
      
      // Other types
      typedef float   Float;
      typedef double  Double;
      typedef void*   Pointer;

      // System dependent types
    #if defined(POSH_OS_WIN32)
	    typedef ::DWORD	  Pid;
	    typedef ::HANDLE	ProcessHandle;
    #elif defined(POSH_OS_LINUX)
	    typedef ::pid_t Pid;
	    typedef ::pid_t	ProcessHandle;
    #endif
    }
  }
}
