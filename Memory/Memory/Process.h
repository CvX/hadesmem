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

#pragma once

// C++ Standard Library
#include <string>

// Boost
#ifdef _MSC_VER
#pragma warning(push, 1)
#endif // #ifdef _MSC_VER
#include <boost/noncopyable.hpp>
#ifdef _MSC_VER
#pragma warning(pop)
#endif // #ifdef _MSC_VER

// Hades
#include "Fwd.h"
#include "Error.h"
#include "Types.hpp"

namespace Hades
{
  namespace Memory
  {
    // Process managing class
    class Process
    {
    public:
      // Process exception type
      class Error : public virtual HadesMemError 
      { };

      // Open process from process ID
      explicit Process(Pid ProcID);

      // Open process from process name
      explicit Process(TString const& ProcName);

      // Open process from window name and class
      Process(TString const& WindowName, TString const& ClassName);

      // Copy constructor
      Process(Process const& MyProcess);

      // Copy assignment
      Process& operator=(Process const& MyProcess);

      // Move constructor
      Process(Process&& MyProcess);

      // Destructor
      ~Process();

      // Move assignment
      Process& operator=(Process&& MyProcess);

      // Get process handle
      ProcessHandle GetHandle() const;
      
      // Get process ID
      Pid GetID() const;

    private:
      // Open process given process id
      void Open(Pid ProcID);

      // Process handle
    #if defined(POSH_OS_WIN32)
      ProcessHandle m_Handle;
    #endif

      // Process ID
      Pid m_ID;
    };
  }
}
