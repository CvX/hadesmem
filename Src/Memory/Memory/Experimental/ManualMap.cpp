// Copyright Joshua Boyce 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

// Hades
#include <HadesMemory/Experimental/ManualMap.hpp>
#include <HadesMemory/Module.hpp>
#include <HadesMemory/MemoryMgr.hpp>
#include <HadesMemory/PeLib/PeLib.hpp>
#include <HadesMemory/Detail/I18n.hpp>
#include <HadesMemory/Detail/WinAux.hpp>
#include <HadesMemory/Detail/EnsureCleanup.hpp>

// C++ Standard Library
#include <memory>
#include <string>
#include <vector>
#include <fstream>
#include <iterator>
#include <iostream>

// Boost
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem/fstream.hpp>

namespace 
{
  std::array<ULONG, 16> const SectionCharacteristicsToProtect = 
  {{
    PAGE_NOACCESS,          /* 0 = NONE */
    PAGE_NOACCESS,          /* 1 = SHARED */
    PAGE_EXECUTE,           /* 2 = EXECUTABLE */
    PAGE_EXECUTE,           /* 3 = EXECUTABLE, SHARED */
    PAGE_READONLY,          /* 4 = READABLE */
    PAGE_READONLY,          /* 5 = READABLE, SHARED */
    PAGE_EXECUTE_READ,      /* 6 = READABLE, EXECUTABLE */
    PAGE_EXECUTE_READ,      /* 7 = READABLE, EXECUTABLE, SHARED */
    PAGE_READWRITE,         /* 8 = WRITABLE */
    PAGE_READWRITE,         /* 9 = WRITABLE, SHARED */
    PAGE_EXECUTE_READWRITE, /* 10 = WRITABLE, EXECUTABLE */
    PAGE_EXECUTE_READWRITE, /* 11 = WRITABLE, EXECUTABLE, SHARED */
    PAGE_READWRITE,         /* 12 = WRITABLE, READABLE */
    PAGE_READWRITE,         /* 13 = WRITABLE, READABLE, SHARED */
    PAGE_EXECUTE_READWRITE, /* 14 = WRITABLE, READABLE, EXECUTABLE */
    PAGE_EXECUTE_READWRITE, /* 15 = WRITABLE, READABLE, EXECUTABLE, SHARED */
  }};
}

namespace HadesMem
{
  // Constructor
  ManualMap::ManualMap(MemoryMgr const& MyMemory) 
    : m_Memory(MyMemory), 
    m_MappedMods()
  { }
      
  // Copy constructor
  ManualMap::ManualMap(ManualMap const& Other)
    : m_Memory(Other.m_Memory), 
    m_MappedMods(Other.m_MappedMods)
  { }
  
  // Copy assignment operator
  ManualMap& ManualMap::operator=(ManualMap const& Other)
  {
    this->m_Memory = Other.m_Memory;
    this->m_MappedMods = Other.m_MappedMods;
    
    return *this;
  }
  
  // Move constructor
  ManualMap::ManualMap(ManualMap&& Other)
    : m_Memory(std::move(Other.m_Memory)), 
    m_MappedMods(std::move(Other.m_MappedMods))
  { }
  
  // Move assignment operator
  ManualMap& ManualMap::operator=(ManualMap&& Other)
  {
    this->m_Memory = std::move(Other.m_Memory);
    this->m_MappedMods = std::move(Other.m_MappedMods);
    
    return *this;
  }
  
  // Destructor
  ManualMap::~ManualMap()
  { }

  // Manually map DLL
  HMODULE ManualMap::InjectDll(std::wstring const& Path, 
      std::string const& Export, 
      InjectFlags Flags) const
  {
    // Do not continue if Shim Engine is enabled for local process, 
    // otherwise it could interfere with the address resolution.
    HMODULE const ShimEngMod = GetModuleHandle(L"ShimEng.dll");
    if (ShimEngMod)
    {
      BOOST_THROW_EXCEPTION(Error() << 
        ErrorFunction("ManualMap::Map") << 
        ErrorString("Shims enabled for local process."));
    }
    
    // String to hold 'real' path to module
    boost::filesystem::path PathReal(Path);
      
    // Check if path resolution was requested
    bool PathResolution = ((Flags & InjectFlag_PathResolution) == 
      InjectFlag_PathResolution);

    // Handle path resolution
    if (PathResolution)
    {
      // Check whether we need to convert the path from a relative to 
      // an absolute
      if (PathReal.is_relative())
      {
        // Convert relative path to absolute path
        PathReal = boost::filesystem::absolute(PathReal, 
          Detail::GetSelfDirPath());
      }
      
      // Ensure target file exists
      // Note: Only performing this check when path resolution is enabled, 
      // because otherwise we would need to perform the check in the context 
      // of the remote process, which is not possible to do without 
      // introducing race conditions and other potential problems. So we just 
      // let LoadLibraryW do the check for us.
      if (!boost::filesystem::exists(PathReal))
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("ManualMap::Map") << 
          ErrorString("Could not find module file."));
      }
    }
    
#if 0
    // If path resolution is disabled, use LoadLibrary to perform the 
    // resolution so the DLL search order is correct.
    if (!PathResolution)
    {
      Detail::EnsureFreeLibrary ModLocal = LoadLibraryEx(PathReal.c_str(), 
        nullptr, DONT_RESOLVE_DLL_REFERENCES);
      if (!ModLocal)
      {
        DWORD const LastError = GetLastError();
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("ManualMap::Map") << 
          ErrorString("Could not load module locally.") << 
          ErrorCodeWinLast(LastError));
      }
      Module ModLocalInfo(m_Memory, ModLocal);
      PathReal = ModLocalInfo.GetPath();
    }
#endif

    // If path resolution is disabled, replicate the Windows DLL search order 
    // to try and find the target.
    // FIXME: Not a complte implementation of the Windows DLL search order 
    // algorithm. The following conditions need to be supported:
    // 1. If a DLL with the same module name is already loaded in memory, the 
    // system checks only for redirection and a manifest before resolving to 
    // the loaded DLL, no matter which directory it is in. The system does not 
    // search for the DLL.
    // 2. If the DLL is on the list of known DLLs for the version of Windows 
    // on which the application is running, the system uses its copy of the 
    // known DLL (and the known DLL's dependent DLLs, if any) instead of 
    // searching for the DLL. For a list of known DLLs on the current system, 
    // see the following registry key: HKEY_LOCAL_MACHINE\SYSTEM\
    // CurrentControlSet\Control\Session Manager\KnownDLLs.
    // 3. If a DLL has dependencies, the system searches for the dependent 
    // DLLs as if they were loaded with just their module names. This is true 
    // even if the first DLL was loaded by specifying a full path.
    // FIXME: Furthermore, this implementation does not search the 16-bit 
    // system directory, nor does it search the current working directory (as 
    // that is only meaningful in the context of the remote process), lastly, 
    // it does not search in %PATH%.
    if (!PathResolution)
    {
      // Get app load dir
      boost::filesystem::path AppLoadDir = m_Memory.GetProcessPath();
      AppLoadDir = AppLoadDir.parent_path();
      
      // Get system dir
      boost::filesystem::path SystemDir;
      wchar_t Temp;
      UINT SysDirLen = GetSystemDirectory(&Temp, 1);
      if (!SysDirLen)
      {
        DWORD const LastError = GetLastError();
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("ManualMap::Map") << 
          ErrorString("Could not get length of system dir.") << 
          ErrorCodeWinLast(LastError));
      }
      std::vector<wchar_t> SysDirTemp(SysDirLen);
      if (!GetSystemDirectory(SysDirTemp.data(), SysDirLen))
      {
        DWORD const LastError = GetLastError();
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("ManualMap::Map") << 
          ErrorString("Could not get system dir.") << 
          ErrorCodeWinLast(LastError));
      }
      SystemDir = SysDirTemp.data();
      
      // Get windows directory
      boost::filesystem::path WindowsDir;
      UINT WinDirLen = GetSystemDirectory(&Temp, 1);
      if (!WinDirLen)
      {
        DWORD const LastError = GetLastError();
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("ManualMap::Map") << 
          ErrorString("Could not get length of windows dir.") << 
          ErrorCodeWinLast(LastError));
      }
      std::vector<wchar_t> WinDirTemp(WinDirLen);
      if (!GetSystemDirectory(WinDirTemp.data(), WinDirLen))
      {
        DWORD const LastError = GetLastError();
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("ManualMap::Map") << 
          ErrorString("Could not get windows dir.") << 
          ErrorCodeWinLast(LastError));
      }
      WindowsDir = WinDirTemp.data();
      
      // Create search list
      std::vector<boost::filesystem::path> SearchDirList;
      SearchDirList.push_back(AppLoadDir);
      SearchDirList.push_back(SystemDir);
      SearchDirList.push_back(WindowsDir);
      
      // Search for target
      boost::filesystem::path ResolvedPath;
      for (auto i = SearchDirList.cbegin(); i != SearchDirList.cend(); ++i)
      {
        boost::filesystem::path const& Current = *i;
        boost::filesystem::path ResolvedPathTemp = boost::filesystem::absolute(
          PathReal, Current);
        if (boost::filesystem::exists(ResolvedPathTemp))
        {
          ResolvedPath = ResolvedPathTemp;
          break;
        }
      }
      
      // Ensure target was found
      if (ResolvedPath.empty())
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("ManualMap::Map") << 
          ErrorString("Could not find module file."));
      }
      
      // Set path
      PathReal = ResolvedPath;
    }

    // Convert path to preferred format
    PathReal.make_preferred();
    
    // Open file for reading
    boost::filesystem::basic_ifstream<char> ModuleFile(PathReal, 
      std::ios::binary | std::ios::ate);
    if (!ModuleFile)
    {
      BOOST_THROW_EXCEPTION(Error() << 
        ErrorFunction("ManualMap::Map") << 
        ErrorString("Could not open image file."));
    }

    // Get file size
    std::streamsize const FileSize = ModuleFile.tellg();

    // Allocate memory to hold file data
    // Doing this rather than copying data into a vector to avoid having to 
    // play with the page protection flags on the heap.
    char* const pBase = static_cast<char*>(VirtualAlloc(nullptr, 
      static_cast<SIZE_T>(FileSize), MEM_COMMIT | MEM_RESERVE, 
      PAGE_READWRITE));
    if (!pBase)
    {
      DWORD const LastError = GetLastError();
      BOOST_THROW_EXCEPTION(Error() << 
        ErrorFunction("ManualMap::Map") << 
        ErrorString("Could not allocate memory for image data.") << 
        ErrorCodeWinLast(LastError));
    }
    Detail::EnsureReleaseRegion const EnsureFreeLocalMod(pBase);

    // Seek to beginning of file
    if (!ModuleFile.seekg(0, std::ios::beg))
    {
      BOOST_THROW_EXCEPTION(Error() << 
        ErrorFunction("ManualMap::Map") << 
        ErrorString("Could not seek to beginning of file."));
    }

    // Read file into memory
    if (!ModuleFile.read(pBase, FileSize))
    {
      BOOST_THROW_EXCEPTION(Error() << 
        ErrorFunction("ManualMap::Map") << 
        ErrorString("Could not read file into memory."));
    }

    // Create memory manager for local proc
    MemoryMgr MyMemoryLocal(GetCurrentProcessId());

    // Ensure file is a valid PE file
    std::wcout << Path << " - Performing PE file format validation." << std::endl;
    PeFile MyPeFile(MyMemoryLocal, pBase, PeFile::FileType_Data);
    DosHeader const MyDosHeader(MyPeFile);
    NtHeaders const MyNtHeaders(MyPeFile);

    // Allocate memory for image
    std::wcout << Path << " - Allocating remote memory for image." << std::endl;
    PVOID const RemoteBase = m_Memory.Alloc(MyNtHeaders.GetSizeOfImage());
    std::wcout << Path << " - Image base address: " << RemoteBase << "." << std::endl;
    std::wcout << Path << " - Image size: " << std::hex << MyNtHeaders.GetSizeOfImage() 
      << std::dec << "." << std::endl;
        
    // Add to list
    // FIXME: This is such a nasty hack. Handle cyclic dependenceis in a 
    // better way.
    m_MappedMods[Path] = reinterpret_cast<HMODULE>(RemoteBase);

    // Get all TLS callbacks
    std::vector<PIMAGE_TLS_CALLBACK> TlsCallbacks;
    TlsDir const MyTlsDir(MyPeFile);
    if (MyTlsDir.IsValid())
    {
      TlsCallbacks = MyTlsDir.GetCallbacks();
      std::for_each(TlsCallbacks.cbegin(), TlsCallbacks.cend(), 
        [&] (PIMAGE_TLS_CALLBACK pCurrent)
      {
        std::wcout << Path << " - TLS Callback: " << pCurrent << std::endl;
      });
    }

    // Process import table
    FixImports(MyPeFile);

    // Process relocations
    FixRelocations(MyPeFile, RemoteBase);

    // Write DOS header to process
    std::wcout << Path << " - Writing DOS header." << std::endl;
    std::wcout << Path << " - DOS Header: " << RemoteBase << std::endl;
    m_Memory.Write(RemoteBase, *reinterpret_cast<PIMAGE_DOS_HEADER>(
      pBase));

    // Write NT headers to process
    PBYTE const NtHeadersStart = reinterpret_cast<PBYTE>(MyNtHeaders.
      GetBase());
    PBYTE const NtHeadersEnd = static_cast<PBYTE>(Section(MyPeFile, 0).GetBase());
    std::vector<BYTE> const PeHeaderBuf(NtHeadersStart, NtHeadersEnd);
    PBYTE const TargetAddr = static_cast<PBYTE>(RemoteBase) + MyDosHeader.
      GetNewHeaderOffset();
    std::wcout << Path << " - Writing NT header." << std::endl;
    std::wcout << Path << " - NT Header: " << static_cast<PVOID>(TargetAddr) << 
      std::endl;
    m_Memory.WriteList(TargetAddr, PeHeaderBuf);

    // Write sections to process
    MapSections(MyPeFile, RemoteBase);

    // Call all TLS callbacks
    std::for_each(TlsCallbacks.cbegin(), TlsCallbacks.cend(), 
      [&] (PIMAGE_TLS_CALLBACK pCallback) 
    {
      std::wcout << Path << " - TLS Callback: " << pCallback << "." << std::endl;
      std::vector<PVOID> TlsCallArgs;
      TlsCallArgs.push_back(0);
      TlsCallArgs.push_back(reinterpret_cast<PVOID>(DLL_PROCESS_ATTACH));
      TlsCallArgs.push_back(RemoteBase);
      MemoryMgr::RemoteFunctionRet const TlsRet = 
        m_Memory.Call(reinterpret_cast<PBYTE>(RemoteBase) + 
        reinterpret_cast<DWORD_PTR>(pCallback), 
        MemoryMgr::CallConv_Default, TlsCallArgs);
      std::wcout << Path << " - TLS Callback Returned: " << TlsRet.GetReturnValue() 
        << "." << std::endl;
    });

    // Calculate module entry point
    PVOID EntryPoint = nullptr;
    DWORD AddressOfEP = MyNtHeaders.GetAddressOfEntryPoint();
    if (AddressOfEP)
    {
      EntryPoint = static_cast<PBYTE>(RemoteBase) + 
        MyNtHeaders.GetAddressOfEntryPoint();
    }
    
    // Print entry point address
    std::wcout << Path << " - Entry Point: " << EntryPoint << "." << std::endl;

    // Call entry point
    if (EntryPoint)
    {
      std::vector<PVOID> EpArgs;
      EpArgs.push_back(0);
      EpArgs.push_back(reinterpret_cast<PVOID>(DLL_PROCESS_ATTACH));
      EpArgs.push_back(RemoteBase);
      MemoryMgr::RemoteFunctionRet const EpRet = m_Memory.Call(EntryPoint, 
        MemoryMgr::CallConv_Default, EpArgs);
      std::wcout << Path << " - Entry Point Returned: " << EpRet.GetReturnValue() 
        << "." << std::endl;
    }

    // Load module as data so we can read the EAT locally
    Detail::EnsureFreeLibrary const LocalMod(LoadLibraryEx(
      Path.c_str(), nullptr, DONT_RESOLVE_DLL_REFERENCES));
    if (!LocalMod)
    {
      DWORD const LastError = GetLastError();
      BOOST_THROW_EXCEPTION(Error() << 
        ErrorFunction("ManualMap::InjectDll") << 
        ErrorString("Could not load module locally.") << 
        ErrorCodeWinLast(LastError));
    }

    // Find target function in module if required
    PVOID ExportAddr = nullptr;
    if (!Export.empty())
    {
      // Get export address locally
      FARPROC const LocalFunc = GetProcAddress(LocalMod, Export.c_str());
      if (!LocalFunc)
      {
        DWORD const LastError = GetLastError();
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("ManualMap::InjectDll") << 
          ErrorString("Could not find target function.") << 
          ErrorCodeWinLast(LastError));
      }

      // Calculate function delta
      LONG_PTR const FuncDelta = reinterpret_cast<DWORD_PTR>(LocalFunc) - 
        reinterpret_cast<DWORD_PTR>(static_cast<HMODULE>(LocalMod));
  
      // Calculate function location in remote process
      FARPROC const RemoteFunc = reinterpret_cast<FARPROC>(
        reinterpret_cast<DWORD_PTR>(RemoteBase) + FuncDelta);
      
      // Get export address
      ExportAddr = reinterpret_cast<PVOID const>(reinterpret_cast<DWORD_PTR>(
        RemoteFunc));
    }
    
    // Print export address
    std::wcout << Path << " - Export Address: " << ExportAddr << "." << std::endl;

    // Call remote export (if specified)
    if (ExportAddr)
    {
      std::vector<PVOID> ExpArgs;
      ExpArgs.push_back(RemoteBase);
      MemoryMgr::RemoteFunctionRet const ExpRet = m_Memory.Call(ExportAddr, 
        MemoryMgr::CallConv_Default, ExpArgs);
      std::wcout << Path << " - Export Returned: " << ExpRet.GetReturnValue() << "." 
        << std::endl;
    }

    // Return pointer to module in remote process
    return reinterpret_cast<HMODULE>(RemoteBase);
  }

  // Map sections
  void ManualMap::MapSections(PeFile& MyPeFile, PVOID RemoteAddr) const
  {
    // Debug output
    std::cout << "Mapping sections." << std::endl;

    // Enumerate all sections
    SectionList Sections(MyPeFile);
    for (auto i = Sections.begin(); i != Sections.end(); ++i)
    {
      // Get section
      Section& Current = *i;

      // Get section name
      std::string const Name(Current.GetName());
      std::cout << "Section Name: " << Name.c_str() << std::endl;

      // Calculate target address for section in remote process
      PVOID const TargetAddr = reinterpret_cast<PBYTE>(RemoteAddr) + 
        Current.GetVirtualAddress();
      std::cout << "Target Address: " << TargetAddr << std::endl;

      // Calculate virtual size of section
      DWORD const VirtualSize = Current.GetVirtualSize(); 
      std::cout << "Virtual Size: " << std::hex << VirtualSize << std::dec 
        << std::endl;

      // Calculate start and end of section data in buffer
      DWORD const SizeOfRawData = Current.GetSizeOfRawData();
      PBYTE const DataStart = static_cast<PBYTE>(MyPeFile.GetBase()) + 
        Current.GetPointerToRawData();
      PBYTE const DataEnd = DataStart + SizeOfRawData;

      // Get section data
      std::vector<BYTE> const SectionData(DataStart, DataEnd);

      // Write section data to process
      if (!SectionData.empty())
      {
        m_Memory.WriteList(TargetAddr, SectionData);
      }

      // Get section characteristics
      DWORD SecCharacteristics = Current.GetCharacteristics();

      // Handle case where no explicit protection is provided. Infer 
      // protection flags from section type.
      if((SecCharacteristics & (IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ | 
        IMAGE_SCN_MEM_WRITE)) == 0)
      {
        if(SecCharacteristics & IMAGE_SCN_CNT_CODE)
        {
          SecCharacteristics |= IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ;
        }

        if(SecCharacteristics & IMAGE_SCN_CNT_INITIALIZED_DATA)
        {
          SecCharacteristics |= IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE;
        }

        if(SecCharacteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA)
        {
          SecCharacteristics |= IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE;
        }
      }

      // Look up protection flags for section
      DWORD SecProtect = SectionCharacteristicsToProtect[
        SecCharacteristics >> 28];

      // Set the proper page protections for this section
      DWORD OldProtect;
      if (!VirtualProtectEx(m_Memory.GetProcessHandle(), TargetAddr, 
        VirtualSize, SecProtect, &OldProtect))
      {
        DWORD const LastError = GetLastError();
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("ManualMap::MapSections") << 
          ErrorString("Could not change page protections for section.") << 
          ErrorCodeWinLast(LastError));
      }
    }
  }

  // Fix imports
  void ManualMap::FixImports(PeFile& MyPeFile) const
  {
    // Get NT headers
    NtHeaders const MyNtHeaders(MyPeFile);

    // Get import dir
    ImportDir const CheckImpDir(MyPeFile);
    if (!CheckImpDir.IsValid())
    {
      // Debug output
      std::cout << "Image has no imports." << std::endl;

      // Nothing more to do
      return;
    }

    // Debug output
    std::cout << "Fixing imports." << std::endl;

    // Loop through all the required modules
    ImportDirList ImportDirs(MyPeFile);
    for (auto i = ImportDirs.begin(); i != ImportDirs.end(); ++i)
    {
      // Get import dir
      ImportDir& MyImportDir = *i;

      // Check for forwarded imports
      // Fixme: Handle forwarded imports
      if (MyImportDir.GetTimeDateStamp())
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("ManualMap::FixImports") << 
          ErrorString("Image has unhandled forwarded imports."));
      }

      // Get module name
      std::string const ModuleName(MyImportDir.GetName());
      std::wstring const ModuleNameW(boost::lexical_cast<std::wstring>(
        ModuleName));
      std::wstring const ModuleNameLowerW(boost::to_lower_copy(
        ModuleNameW));
      std::cout << "Module Name: " << ModuleName << "." << std::endl;
      
      // Check whether dependent module is already loaded
      boost::optional<Module> MyModule;
      ModuleList Modules(m_Memory);
      for (auto j = Modules.begin(); j != Modules.end(); ++j)
      {
        Module& Current = *j;
        if (boost::to_lower_copy(Current.GetName()) == ModuleNameLowerW || 
          boost::to_lower_copy(Current.GetPath()) == ModuleNameLowerW)
        {
          // Fixme: Correctly implement the DLL search order and compare by 
          // absolute path rather than by name.
          // http://msdn.microsoft.com/en-us/library/ms682586.aspx
          if (MyModule)
          {
            std::cout << "WARNING! Found two modules with the same name. You "
              "may experience unexpected behaviour." << std::endl;
          }
          
          MyModule = *j;
        }
      }
      
      // Current module base
      HMODULE CurModBase = nullptr;

      // If dependent module is not yet loaded then inject it
      if (!MyModule)
      {
        // Check whether dependent module is already manually mapped
        auto const MappedModIter = m_MappedMods.find(ModuleNameW);
        if (MappedModIter != m_MappedMods.end())
        {
          std::cout << "Found existing mapped instance of dependent DLL." << 
            std::endl;
          CurModBase = MappedModIter->second;
        }
        else
        {
          // Inject dependent DLL
          std::cout << "Manually mapping dependent DLL." << std::endl;
          try
          {
            std::cout << "Attempting without path resolution." << std::endl;
            CurModBase = InjectDll(ModuleNameW);
          }
          catch (boost::exception const& e)
          {
            if (std::string const* x = boost::get_error_info<ErrorString>(e))
            {
              if (*x == "Could not open image file." || 
                *x == "Could not find module file.")
              {
                std::cout << "Failure." << std::endl;
                std::cout << "Attempting with path resolution." << std::endl;
                CurModBase = InjectDll(ModuleNameW, "", 
                  InjectFlag_PathResolution);
              }
              else
              {
                throw;
              }
            }          
          }
        }
      }
      else
      {
        CurModBase = MyModule->GetHandle();
      }
      
      // Load dependent module locally for export enumeration
      // FIXME: This is wrong! In the case of forwarded exports this will 
      // cause GetProcAddress to return a pointer in a different module which 
      // does not map correctly to the remote process.
      Detail::EnsureFreeLibrary DepModLocal = LoadLibraryEx(
        ModuleNameW.c_str(), nullptr, DONT_RESOLVE_DLL_REFERENCES);
      if (!DepModLocal)
      {
        DWORD const LastError = GetLastError();
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("ManualMap::FixImports") << 
          ErrorString("Could not load dependent module locally.") << 
          ErrorCodeWinLast(LastError));
      }

      // Loop over import thunks for current module
      ImportThunkList ImportThunks(MyPeFile, MyImportDir.GetFirstThunk());
      for (auto j = ImportThunks.begin(); j != ImportThunks.end(); ++j)
      {
        // Get import thunk
        ImportThunk& ImpThunk = *j;

        // Get function address in remote process
        FARPROC FuncAddr = 0;
        if (ImpThunk.ByOrdinal())
        {
          // Get name of function
          std::cout << "Function Ordinal: " << ImpThunk.GetOrdinal() << "." 
            << std::endl;

          // Get export address locally then convert to remote address
          Module CurModule(m_Memory, DepModLocal);
          FuncAddr = CurModule.FindProcedure(ImpThunk.GetOrdinal());
          
          // Convert address if found
          if (FuncAddr)
          {
            LONG_PTR const FuncDelta = reinterpret_cast<DWORD_PTR>(FuncAddr) - 
              reinterpret_cast<DWORD_PTR>(static_cast<HMODULE>(DepModLocal));
        
            FuncAddr = reinterpret_cast<FARPROC>(reinterpret_cast<DWORD_PTR>(
              CurModBase) + FuncDelta);
          }
        }
        else
        {
          // Get name of function
          std::string const ImpName(ImpThunk.GetName());
          std::cout << "Function Name: " << ImpName << "." << std::endl;

          // Get export address locally then convert to remote address
          Module CurModule(m_Memory, DepModLocal);
          FuncAddr = CurModule.FindProcedure(ImpThunk.GetName());

          // Convert address if found
          if (FuncAddr)
          {
            LONG_PTR const FuncDelta = reinterpret_cast<DWORD_PTR>(FuncAddr) - 
              reinterpret_cast<DWORD_PTR>(static_cast<HMODULE>(DepModLocal));
        
            FuncAddr = reinterpret_cast<FARPROC>(reinterpret_cast<DWORD_PTR>(
              CurModBase) + FuncDelta);
          }
        }

        // Ensure function was found
        if (!FuncAddr)
        {
          BOOST_THROW_EXCEPTION(Error() << 
            ErrorFunction("ManualMap::FixImports") << 
            ErrorString("Could not find current import."));
        }

        // Set function address
        ImpThunk.SetFunction(reinterpret_cast<DWORD_PTR>(FuncAddr));
      }
    } 
  }

  // Fix relocations
  void ManualMap::FixRelocations(PeFile& MyPeFile, PVOID pRemoteBase) const
  {
    // Get NT headers
    NtHeaders const MyNtHeaders(MyPeFile);

    // Get import data dir size and address
    DWORD const RelocDirSize = MyNtHeaders.GetDataDirectorySize(NtHeaders::
      DataDir_BaseReloc);
    PIMAGE_BASE_RELOCATION pRelocDir = 
      static_cast<PIMAGE_BASE_RELOCATION>(MyPeFile.RvaToVa(MyNtHeaders.
      GetDataDirectoryVirtualAddress(NtHeaders::DataDir_BaseReloc)));
    if (!RelocDirSize || !pRelocDir)
    {
      // Debug output
      std::cout << "Image has no relocations." << std::endl;

      // Nothing more to do
      return;
    }

    // Get end of reloc dir
    PVOID pRelocDirEnd = reinterpret_cast<PBYTE>(pRelocDir) + RelocDirSize;

    // Debug output
    std::cout << "Fixing relocations." << std::endl;

    // Get image base
    ULONG_PTR const ImageBase = MyNtHeaders.GetImageBase();

    // Calcuate module delta
    LONG_PTR const Delta = reinterpret_cast<ULONG_PTR>(pRemoteBase) - 
      ImageBase;

    // Ensure we don't read into invalid data
    while (pRelocDir < pRelocDirEnd && pRelocDir->SizeOfBlock > 0)
    {
      // Get base of reloc dir
      PBYTE const RelocBase = static_cast<PBYTE>(MyPeFile.RvaToVa(
        pRelocDir->VirtualAddress));

      // Get number of relocs
      DWORD const NumRelocs = (pRelocDir->SizeOfBlock - sizeof(
        IMAGE_BASE_RELOCATION)) / sizeof(WORD); 

      // Get pointer to reloc data
      PWORD pRelocData = reinterpret_cast<PWORD>(pRelocDir + 1);

      // Loop over all relocation entries
      for(DWORD i = 0; i < NumRelocs; ++i, ++pRelocData) 
      {
        // Get reloc data
        BYTE RelocType = *pRelocData >> 12;
        WORD Offset = *pRelocData & 0xFFF;

        // Process reloc
        switch (RelocType)
        {
        case IMAGE_REL_BASED_ABSOLUTE:
          break;

        case IMAGE_REL_BASED_HIGHLOW:
          *reinterpret_cast<DWORD32*>(RelocBase + Offset) += 
            static_cast<DWORD32>(Delta);
          break;

        case IMAGE_REL_BASED_DIR64:
          *reinterpret_cast<DWORD64*>(RelocBase + Offset) += Delta;
          break;

        default:
          std::cout << "Unsupported relocation type: " << RelocType << 
            std::endl;

          BOOST_THROW_EXCEPTION(Error() << 
            ErrorFunction("ManualMap::FixRelocations") << 
            ErrorString("Unsuppported relocation type."));
        }
      }

      // Advance to next reloc info block
      pRelocDir = reinterpret_cast<PIMAGE_BASE_RELOCATION>(pRelocData); 
    }
  }
  
  // Equality operator
  bool ManualMap::operator==(ManualMap const& Rhs) const
  {
    return m_Memory == Rhs.m_Memory;
  }
  
  // Inequality operator
  bool ManualMap::operator!=(ManualMap const& Rhs) const
  {
    return !(*this == Rhs);
  }
}
