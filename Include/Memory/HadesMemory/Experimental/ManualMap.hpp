// Copyright Joshua Boyce 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#pragma once

// Hades
#include <HadesMemory/MemoryMgr.hpp>
#include <HadesMemory/Detail/Error.hpp>
#include <HadesMemory/PeLib/PeFile.hpp>
#include <HadesMemory/Detail/Config.hpp>

// C++ Standard Library
#include <map>
#include <string>
#include <utility>

// Windows API
#include <Windows.h>

namespace HadesMem
{
  // Manual mapping class
  class ManualMap
  {
  public:
    // ManualMap exception type
    class Error : public virtual HadesMemError 
    { };

    // Constructor
    explicit ManualMap(MemoryMgr const& MyMemory);
      
    // Copy constructor
    ManualMap(ManualMap const& Other);
    
    // Copy assignment operator
    ManualMap& operator=(ManualMap const& Other);
    
    // Move constructor
    ManualMap(ManualMap&& Other);
    
    // Move assignment operator
    ManualMap& operator=(ManualMap&& Other);
    
    // Destructor
    ~ManualMap();
    
    // Manual mapping flags
    enum InjectFlags
    {
      InjectFlag_None, 
      InjectFlag_PathResolution
    };

    // Manually map DLL
    HMODULE InjectDll(std::wstring const& Path, 
      std::string const& Export = "", 
      InjectFlags Flags = InjectFlag_None) const;
    
    // Equality operator
    bool operator==(ManualMap const& Rhs) const;
    
    // Inequality operator
    bool operator!=(ManualMap const& Rhs) const;

  private:
    // Map sections
    void MapSections(PeFile& MyPeFile, PVOID RemoteAddr) const;

    // Fix imports
    void FixImports(PeFile& MyPeFile) const;

    // Fix relocations
    void FixRelocations(PeFile& MyPeFile, PVOID RemoteAddr) const;

    // MemoryMgr instance
    MemoryMgr m_Memory;
    
    // Map of mapped DLLs and their bases
    mutable std::map<std::wstring, HMODULE> m_MappedMods;
  };
}
