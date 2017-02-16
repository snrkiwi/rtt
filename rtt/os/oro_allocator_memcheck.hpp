/***************************************************************************
  tag: Johannes Meyer  Tue Jun 23 16:52:58 CEST 2015  oro_allocator_memcheck.hpp

                        oro_allocator_memcheck.hpp -  description
                        --------------------------
    begin                : Tue Jun 23 2015
    copyright            : (C) 2009 Peter Soetens
    email                : johannes@intermodalics.eu

 ***************************************************************************
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public                   *
 *   License as published by the Free Software Foundation;                 *
 *   version 2 of the License.                                             *
 *                                                                         *
 *   As a special exception, you may use this file as part of a free       *
 *   software library without restriction.  Specifically, if other files   *
 *   instantiate templates or use macros or inline functions from this     *
 *   file, or you compile this file and link it with other files to        *
 *   produce an executable, this file does not by itself cause the         *
 *   resulting executable to be covered by the GNU General Public          *
 *   License.  This exception does not however invalidate any other        *
 *   reasons why the executable file might be covered by the GNU General   *
 *   Public License.                                                       *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public             *
 *   License along with this library; if not, write to the Free Software   *
 *   Foundation, Inc., 59 Temple Place,                                    *
 *   Suite 330, Boston, MA  02111-1307  USA                                *
 *                                                                         *
 ***************************************************************************/

#ifndef ORO_ALLOCATOR_MEMCHECK_HPP
#define ORO_ALLOCATOR_MEMCHECK_HPP

#include <iostream>
#include <cstdlib>

namespace RTT { namespace os {

/**********************************************************************************************************************
 * Configuration variables
 *********************************************************************************************************************/

// Whether checking is enabled. Defaults to true
extern bool memcheck_enabled;
//extern std::ostream *memcheck_error_stream;
//extern std::ostream *memcheck_debug_stream;
extern std::size_t memcheck_memory_usage_by_allocator_warning_limit;
extern std::size_t memcheck_nr_of_allocations_by_allocator_warning_limit;

/**********************************************************************************************************************
 * Allocate/deallocate hooks
 *********************************************************************************************************************/
void oro_allocator_memcheck_allocate(void *p, std::size_t n);
void oro_allocator_memcheck_deallocate(void *p, std::size_t n);

}} // namespace

#endif // ORO_ALLOCATOR_MEMCHECK_HPP
