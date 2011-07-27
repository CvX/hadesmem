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

// C++ Standard Library
#include <string>
#include <vector>

// Windows API
#include <Windows.h>

namespace HadesMem
{
  // DLL injection class
  class Injector
  {
  public:
    // Injector exception type
    class Error : public virtual HadesMemError 
    { };

    // Constructor
    explicit Injector(MemoryMgr const& MyMemory);
      
    // Copy constructor
    Injector(Injector const& Other);
    
    // Copy assignment operator
    Injector& operator=(Injector const& Other);
    
    // Move constructor
    Injector(Injector&& Other);
    
    // Move assignment operator
    Injector& operator=(Injector&& Other);
    
    // Destructor
    ~Injector();
    
    // Injection flags
    enum InjectFlags
    {
      InjectFlag_None, 
      InjectFlag_PathResolution
    };

    // Inject DLL
    HMODULE InjectDll(std::wstring const& Path, 
      InjectFlags Flags = InjectFlag_None) const;
      
    // Free DLL
    void FreeDll(HMODULE Module) const;
    
    // Call export
    MemoryMgr::RemoteFunctionRet CallExport(HMODULE ModuleRemote, 
      std::string const& Export) const;

  private:
    // MemoryMgr instance
    MemoryMgr m_Memory;
  };
    
  // Return data for CreateAndInject
  class CreateAndInjectData
  {
  public:
    CreateAndInjectData(MemoryMgr const& MyMemory, HMODULE Module, 
      DWORD_PTR ExportRet, DWORD ExportLastError) 
      : m_Memory(MyMemory), 
      m_Module(Module), 
      m_ExportRet(ExportRet), 
      m_ExportLastError(ExportLastError)
    { }
    
    CreateAndInjectData(CreateAndInjectData const& Other)
      : m_Memory(Other.m_Memory), 
      m_Module(Other.m_Module), 
      m_ExportRet(Other.m_ExportRet), 
      m_ExportLastError(Other.m_ExportLastError)
    { }
    
    MemoryMgr GetMemoryMgr() const
    {
      return m_Memory;
    }
    
    HMODULE GetModule() const
    {
      return m_Module;
    }
    
    DWORD_PTR GetExportRet() const
    {
      return m_ExportRet;
    }
    
    DWORD GetExportLastError() const
    {
      return m_ExportLastError;
    }
    
  private:
    MemoryMgr m_Memory;
    HMODULE m_Module;
    DWORD_PTR m_ExportRet;
    DWORD m_ExportLastError;
  };
  
  // Create process (as suspended) and inject DLL
  CreateAndInjectData CreateAndInject(
    std::wstring const& Path, 
    std::wstring const& WorkDir, 
    std::vector<std::wstring> const& Args, 
    std::wstring const& Module, 
    std::string const& Export, 
    Injector::InjectFlags Flags = Injector::InjectFlag_None);
}
