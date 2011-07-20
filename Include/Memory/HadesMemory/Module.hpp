/*
This file is part of HadesMem.
Copyright (C) 2011 Joshua Boyce (a.k.a. RaptorFactor).
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

#pragma once

// Hades
#include <HadesMemory/MemoryMgr.hpp>
#include <HadesMemory/Detail/Error.hpp>
#include <HadesMemory/Detail/EnsureCleanup.hpp>

// C++ Standard Library
#include <string>
#include <vector>

// Boost
#include <boost/optional.hpp>
#include <boost/iterator/iterator_facade.hpp>

// Windows API
#include <Windows.h>
#include <TlHelp32.h>

namespace HadesMem
{
  // Module managing class
  class Module
  {
  public:
    // Module exception type
    class Error : public virtual HadesMemError
    { };

    // Find module by handle
    Module(MemoryMgr const& MyMemory, HMODULE Handle);

    // Find module by name
    Module(MemoryMgr const& MyMemory, std::wstring const& ModuleName);

    // Create module
    Module(MemoryMgr const& MyMemory, MODULEENTRY32 const& ModuleEntry);
      
    // Copy constructor
    Module(Module const& Other);
    
    // Copy assignment operator
    Module& operator=(Module const& Other);
    
    // Move constructor
    Module(Module&& Other);
    
    // Move assignment operator
    Module& operator=(Module&& Other);
    
    // Destructor
    ~Module();

    // Get module handle
    HMODULE GetHandle() const;
    
    // Get module size
    DWORD GetSize() const;

    // Get module name
    std::wstring GetName() const;
    
    // Get module path
    std::wstring GetPath() const;
      
    // Find procedure by name
    FARPROC FindProcedure(std::string const& Name) const;
    
    // Find procedure by ordinal
    FARPROC FindProcedure(WORD Ordinal) const;
    
    // Equality operator
    bool operator==(Module const& Rhs) const;
    
    // Inequality operator
    bool operator!=(Module const& Rhs) const;

  private:
    // Memory instance
    MemoryMgr m_Memory;

    // Module base address
    HMODULE m_Base;
    // Module size
    DWORD m_Size;
    // Module name
    std::wstring m_Name;
    // Module path
    std::wstring m_Path;
  };
  
  // Get remote module handle
  HMODULE GetRemoteModuleHandle(MemoryMgr const& MyMemory, 
    LPCWSTR ModuleName);

  // Module iterator
  class ModuleIter : public boost::iterator_facade<ModuleIter, Module, 
    boost::forward_traversal_tag>
  {
  public:
    // Module iterator error class
    class Error : public virtual HadesMemError
    { };

    // Constructor
    ModuleIter();
    
    // Constructor
    ModuleIter(class ModuleList& Parent);
    
  private:
    // Give Boost.Iterator access to internals
    friend class boost::iterator_core_access;

    // Increment iterator
    void increment();
    
    // Check iterator for equality
    bool equal(ModuleIter const& Rhs) const;

    // Dereference iterator
    Module& dereference() const;

    // Parent list instance
    class ModuleList* m_pParent;
    
    // Module number
    DWORD m_Number;
    
    // Current module instance
    mutable boost::optional<Module> m_Current;
  };
  
  // Module enumeration class
  class ModuleList
  {
  public:
    // ModuleList exception type
    class Error : public virtual HadesMemError
    { };
    
    // Module list iterator types
    typedef ModuleIter iterator;
    
    // Constructor
    ModuleList(MemoryMgr const& MyMemory);
    
    // Move constructor
    ModuleList(ModuleList&& Other);
    
    // Move assignment operator
    ModuleList& operator=(ModuleList&& Other);
    
    // Get start of module list
    iterator begin();
    
    // Get end of module list
    iterator end();
    
  protected:
    // Disable copying and copy-assignment
    ModuleList(ModuleList const& Other);
    ModuleList& operator=(ModuleList const& Other);
    
  private:
    // Give ModuleIter access to internals
    friend class ModuleIter;
    
    // Get module from cache by number
    boost::optional<Module&> GetByNum(DWORD Num);
    
    // Memory instance
    MemoryMgr m_Memory;
    
    // Snapshot handle
    Detail::EnsureCloseSnap m_Snap;
    
    // Module cache
    std::vector<Module> m_Cache;
  };
}
