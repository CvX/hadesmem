// Copyright Joshua Boyce 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#pragma once

#include <HadesMemory/MemoryMgr.hpp>
#include <HadesMemory/Detail/Error.hpp>
#include <HadesMemory/PeLib/PeFile.hpp>
#include <HadesMemory/PeLib/ExportDir.hpp>

#include <map>
#include <string>
#include <vector>

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
    
    // Equality operator
    bool operator==(ManualMap const& Rhs) const;
    
    // Inequality operator
    bool operator!=(ManualMap const& Rhs) const;

    // Manually map DLL
    HMODULE InjectDll(std::wstring const& Path, 
      std::wstring const& ParentPath = std::wstring(), 
      std::string const& Export = std::string()) const;
    
  private:
    // Resolve path as per Windows loader
    std::wstring ResolvePath(std::wstring const& Path, 
      std::wstring const& Parent = std::wstring()) const;
    
    // Perform ApiSetSchema redirection
    std::wstring ResolveApiSetSchema(std::wstring const& Path, 
      std::wstring const& Parent) const;
    
    // Add module to cache
    void AddToCache(std::wstring const& Path, HMODULE Base) const;
    
    // Find module in cache
    HMODULE LookupCache(std::wstring const& Path) const;
    
    // Map PE headers
    void MapHeaders(PeFile const& MyPeFile, PVOID RemoteBase) const;
    
    // Get local PE file memory
    AllocAndFree OpenFile(std::wstring const& Path) const;

    // Fix relocations
    void FixRelocations(PeFile const& MyPeFile, PVOID RemoteBase) const;
    
    // Map sections
    void MapSections(PeFile const& MyPeFile, PVOID RemoteBase) const;

    // Fix imports
    void FixImports(PeFile const& MyPeFile, 
      std::wstring const& ParentPath) const;
      
    // Resolve export
    FARPROC ResolveExport(Export const& E, 
      std::wstring const& ParentPath) const;
    
    // Find export by name
    Export FindExport(PeFile const& MyPeFile, std::string const& Name) const;
  
    // Call initialization routines
    void CallInitRoutines(PeFile const& MyPeFile, 
      std::vector<PIMAGE_TLS_CALLBACK> const& TlsCallbacks, 
      PVOID RemoteBase) const;
    
    // Call export
    void CallExport(std::wstring const& FullPath, std::string const& Export, 
      PVOID RemoteBase) const;
    
    // API Schema list types
    typedef std::map<std::wstring, std::wstring> ApiSchemaDefaultMap;
    typedef std::pair<std::wstring, std::wstring> ApiSchemaExceptionPair;
    typedef std::vector<ApiSchemaExceptionPair> ApiSchemaExceptionList;
    typedef std::map<std::wstring, ApiSchemaExceptionList> 
      ApiSchemaExceptionMap;
    
    // Initialize API set schema
    void InitializeApiSchema() const;

    // MemoryMgr instance
    MemoryMgr m_Memory;
    
    // Manually mapped modules
    mutable std::map<std::wstring, HMODULE> m_MappedMods;
    
    // API set schema tables
    mutable ApiSchemaDefaultMap m_ApiSchemaDefaults;
    mutable ApiSchemaExceptionMap m_ApiSchemaExceptions;
  };
}

#if 0
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
      InjectFlags Flags = InjectFlag_None, 
      std::wstring const& Parent = L"") const;
    
    // Equality operator
    bool operator==(ManualMap const& Rhs) const;
    
    // Inequality operator
    bool operator!=(ManualMap const& Rhs) const;

  private:
    // Map sections
    void MapSections(PeFile& MyPeFile, PVOID RemoteAddr) const;

    // Fix imports
    void FixImports(PeFile& MyPeFile, std::wstring const& ParentPath) const;

    // Fix relocations
    void FixRelocations(PeFile& MyPeFile, PVOID RemoteAddr) const;
    
    // Perform path resolution
    std::wstring ResolvePath(std::wstring const& Path, 
      bool PathResolution) const;
      
    // Resolve export
    FARPROC ResolveExport(Export const& E, 
      std::wstring const& ParentPath) const;
    
    // Find export by name
    Export FindExport(PeFile const& MyPeFile, std::string const& Name) const;
    
    // API Schema types (undocumented)
    struct ApiSetMapHeader
    {
      DWORD Version;
      DWORD NumModules;
    };
    struct ApiSetModuleEntry
    {
      DWORD OffsetToName;
      WORD NameSize;
      DWORD OffsetOfHosts;
    };
    struct ApiSetModuleHostsHeader
    {
      DWORD NumHosts;
    };
    struct ApiSetModuleHost
    {
      DWORD OffsetOfImportingName;
      WORD ImportingNameSize;
      DWORD OffsetOfHostName;
      WORD HostNameSize;
    };
    
    // API Schema list types
    typedef std::map<std::wstring, std::wstring> ApiSchemaDefaultMap;
    typedef std::pair<std::wstring, std::wstring> ApiSchemaExceptionPair;
    typedef std::vector<ApiSchemaExceptionPair> ApiSchemaExceptionList;
    typedef std::map<std::wstring, ApiSchemaExceptionList> 
      ApiSchemaExceptionMap;
    
    // Initialize API set schema
    void InitializeApiSchema() const;

    // MemoryMgr instance
    MemoryMgr m_Memory;
    
    // Manually mapped modules
    mutable std::map<std::wstring, HMODULE> m_MappedMods;
    
    // API set schema tables
    mutable ApiSchemaDefaultMap m_ApiSchemaDefaults;
    mutable ApiSchemaExceptionMap m_ApiSchemaExceptions;
  };
}
#endif
