// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

namespace hadesmem
{
class Process;
class PeFile;
}

void DumpExports(hadesmem::Process const& process,
                 hadesmem::PeFile const& pe_file);
