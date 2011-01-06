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

#include <posh.h>
#if defined(POSH_OS_LINUX)

// POSIX
#include <unistd.h>
#include <elf.h>

// C++ Standard Library
#include <string>
#include <sstream>
#include <fstream>

// Boost
#include <boost/foreach.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/array.hpp>

// Hades
#include "Process.h"
#include "Types.h"

namespace Hades
{
  namespace Memory
  {
    // Open process from process id
    Process::Process(Pid ProcID) : m_ID(ProcID) 
    {
      // Open process.
      Open(m_ID);
    }

    // Open process from process name.
    Process::Process(TString const& ProcName) : m_ID(0)
    {
      namespace bf = boost::filesystem;

      // We can assume that every directory in the /proc filesystem
      // consisting of numbers only (the PID) is a process.
      bf::directory_iterator it_end;
      for(bf::directory_iterator it("/proc/"); it != it_end; ++it)
      {
        
        // It's a file, ignore.
        if(!bf::is_directory(it->status()))
          continue;

        // If the directory name doesn't consist of numeric values.
        // only, ignore.
        BOOST_FOREACH(char cur, it->path().filename())
        {
          if(cur < '0' || cur > '9')
            continue;
        }

        // Great, this has to be a process. Open stat file for reading.
        bf::ifstream statFile(it->path() / "stat");
        if(!statFile.is_open() || !statFile.good())
        {
          BOOST_THROW_EXCEPTION(Error() <<
            ErrorFunction("Process::Process") << 
            ErrorString("Failed to open stat file for reading."));
        }
         
        // Grab pid and name from the stat file.
        Pid pid;
        statFile >> pid;

        // Name is hold by braces.
        std::string temp;
        statFile >> temp;
        while(*(temp.end() - 1) != ')')
        {
          //Append a space as it gets eaten by the extractor.
          temp.append(" ");

          std::string chunk;
          statFile >> chunk;
          temp.append(chunk);
        }

        // Remove braces and implicitly convert to TString.
        TString name(name.begin() + 1, name.end() - 1);

        // Check if we found our process.
        if(name == ProcName)
        {
          // Open process
          m_ID = pid;
          Open(m_ID);

          return;
        }

      }

      // Process wasn't found.
      BOOST_THROW_EXCEPTION(Error() << 
        ErrorFunction("Process::Process") << 
        ErrorString("Could not find process."));
    }

    // Open process from window name and class
    Process::Process(TString const& WindowName, TString const& ClassName)
      : m_ID(0) 
    {
      // Not implemented.
      BOOST_THROW_EXCEPTION(Error() <<
        ErrorFunction("Process::Process") << 
        ErrorString("Not yet implemented."));    
    }

    // Copy constructor
    Process::Process(Process const& MyProcess) : m_ID(MyProcess.m_ID)
    {
      Open(m_ID);
    }

    ~Process()
    {
      // Detach from process.
      long ec = ptrace(PTRACE_DETACH, m_ID, nullptr, nullptr);
      if(ec == -1)
      {
        auto errCode = MakeErrorCodeSys();

        BOOST_THROW_EXCEPTION(Error() <<
          ErrorFunction("Process::~Process") << 
          ErrorString("Could not detach from process.") <<
          ErrorCodeSys(errCode));     
      }
    }

    // Copy assignment
    Process& Process::operator=(Process const& MyProcess)
    {
      m_ID = MyProcess.m_ID;
      Open(m_ID);

      return *this;
    }

    // Open process given process id
    void Process::Open(Pid ProcID)
    {
      // Attach to process.
      long ec = ptrace(PTRACE_ATTACH, m_ID, nullptr, nullptr);
      if(ec == -1)
      {
        auto errCode = MakeErrorCodeSys();

        BOOST_THROW_EXCEPTION(Error() <<
          ErrorFunction("Process::Open") << 
          ErrorString("Could not attach to process.") <<
          ErrorCodeSys(errCode));     
      }

      // Check if the current process is 64bit.
      bool isSelf64 = sizeof(void*) == 8;

      // Check if the other process is 64bit
      std::stringstream exePath;
      exePath << "/proc/" << ProcID << "/exe";

      std::ifstream exe(exePath.str().c_str());
      if(!exe.is_open() || !exe.good())
      {
        BOOST_THROW_EXCEPTION(Error() <<
        ErrorFunction("Process::Open") << 
        ErrorString("Could not open executeable for reading."));
      }
  
      // Read ELF identification bytes and compare binary class.
      char ident[EI_NIDENT];
      exe.read(ident, EI_NIDENT);
      bool isOther64 = ident[EI_CLASS] == ELFCLASS64;

      // Ensure both executeables are for the same architecture
      if (isSelf64 != isOther64)
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("Process::Open") << 
          ErrorString("Cross-architecture process manipulation is "
          "currently unsupported."));
      }
    }

    // Get process handle (alias for pid)
    ProcessHandle Process::GetHandle() const
    {
      return m_ID;
    }

    // Get process ID
    Pid Process::GetID() const
    {
      return m_ID;
    }
  }
}

#endif //POSH_OS_LINUX
