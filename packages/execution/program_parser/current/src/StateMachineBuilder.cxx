/***************************************************************************
  tag: Peter Soetens  Tue Jul 20 15:13:58 CEST 2004  StateMachineBuilder.cxx

                        StateMachineBuilder.cxx -  description
                           -------------------
    begin                : Tue July 20 2004
    copyright            : (C) 2004 Peter Soetens
    email                : peter.soetens@mech.kuleuven.ac.be

 ***************************************************************************
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Lesser General Public            *
 *   License as published by the Free Software Foundation; either          *
 *   version 2.1 of the License, or (at your option) any later version.    *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with this library; if not, write to the Free Software   *
 *   Foundation, Inc., 59 Temple Place,                                    *
 *   Suite 330, Boston, MA  02111-1307  USA                                *
 *                                                                         *
 ***************************************************************************/


#include "execution/StateMachineBuilder.hpp"
#include "execution/ParsedStateMachine.hpp"
#include <assert.h>

namespace ORO_Execution {
  StateMachineBuilder::StateMachineBuilder(
    ParsedStateMachine* tc )
    : templatecontext( tc )
  {
  }

  StateMachineBuilder::~StateMachineBuilder()
  {
      // template is owned by __states TC.
      //delete templatecontext;
  }

  ParsedStateMachine* StateMachineBuilder::build(bool instantiate)
  {
    std::map<const DataSourceBase*, DataSourceBase*> dsmap;
    return templatecontext->copy( dsmap, instantiate );
  }
}
