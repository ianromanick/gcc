// 2003-05-13 Benjamin Kosnik  <bkoz@redhat.com>

// Copyright (C) 2003 Free Software Foundation, Inc.
//
// This file is part of the GNU ISO C++ Library.  This library is free
// software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the
// Free Software Foundation; either version 2, or (at your option)
// any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License along
// with this library; see the file COPYING.  If not, write to the Free
// Software Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307,
// USA.

// 27.8.1.4 Overridden virtual functions

#include <fstream>
#include <locale>
#include <testsuite_hooks.h>

class state_codecvt : public std::codecvt<char, char, std::mbstate_t>
{
protected:
  int
  do_encoding() const throw()
  { return -1; }
};

void test03()
{
  using namespace std;
  bool test __attribute__((unused)) = true;

  locale loc_s(locale::classic(), new state_codecvt);
  filebuf ob;
  ob.pubimbue(loc_s);
  VERIFY( ob.getloc() == loc_s );

  // 2 "if encoding of current locale is state dependent" fails...
  locale loc_c = locale::classic();
  locale ret = ob.pubimbue(loc_s);
  VERIFY( ob.getloc() == loc_s );
}

int main() 
{
  test03();
  return 0;
}
