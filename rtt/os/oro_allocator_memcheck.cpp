/***************************************************************************
  tag: Johannes Meyer  Tue Jun 23 16:52:58 CEST 2015  oro_allocator_memcheck.cpp

                        oro_allocator_memcheck.cpp -  description
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

#include "oro_allocator_memcheck.hpp"

#include <rtt/os/TimeService.hpp>
#include <rtt/os/Mutex.hpp>
#include <rtt/os/MutexLock.hpp>
#include <rtt/Logger.hpp>

#include <boost/functional/hash.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>    // with I/O
#include <execinfo.h>
#include <cxxabi.h>
#include <unistd.h>      // getpid()

#include <map>
#include <iomanip>

namespace bpt	= boost::posix_time;


namespace RTT { namespace os {

void formatTime(const bpt::ptime t, std::string& t_s)
{
    static const size_t START = strlen("YYYYMMDDTHHMMSS,");

    // format now as "YYYYMMDDTHHMMSS.ffffff"
    t_s = bpt::to_iso_string(t);          // YYYYMMDDTHHMMSS,fffffffff
    t_s.replace(START-1, 1, ".");         // replace "," with "."
    t_s.erase(t_s.size()-3, 3);         // from nanosec to microsec
}
void getNow(bpt::ptime& now, std::string& now_s)
{
    now = bpt::microsec_clock::local_time();
    formatTime(now, now_s);
}

/**********************************************************************************************************************
 * Configuration variables
 *********************************************************************************************************************/

bool memcheck_enabled = true;

std::ostream *memcheck_error_stream = &std::cerr;
std::ostream *memcheck_debug_stream = 0;
// or...
//RTT::Logger *memcheck_error_stream = RTT::Logger::Instance();
//RTT::Logger *memcheck_error_stream = 0;

std::size_t memcheck_memory_usage_by_allocator_warning_limit = 8 * 1024;
std::size_t memcheck_nr_of_allocations_by_allocator_warning_limit = 1000;

/**********************************************************************************************************************
 * Internal bookkeeping
 *********************************************************************************************************************/
typedef std::size_t hash_type; // used as a hash over backtraces
typedef std::vector<std::string> backtrace_symbols_type;

typedef void *pointer;
struct AllocData
{
    pid_t           pid;
    pthread_t       pthreadid;
    hash_type       allocator;
    std::size_t     size;
    bpt::ptime      time;
    AllocData() :
        pid(0),
        pthreadid(0),
        allocator(0),
        size(0),
        time(bpt::not_a_date_time)
    {}
    AllocData(pid_t p, pthread_t d, hash_type a, std::size_t s, bpt::ptime t) :
        pid(p),
        pthreadid(d),
        allocator(a),
        size(s),
        time(t)
    {}
};

static struct MemcheckStatistics
{
    RTT::os::Mutex mutex;
    std::map<hash_type, std::size_t> nr_of_allocations_by_allocator;
    std::map<hash_type, std::size_t> allocated_memory_by_allocator;
    std::map<pointer, AllocData> allocations_by_pointer;
    std::map<pointer, hash_type> deallocator_by_pointer;
    std::size_t total_nr_of_allocations;
    std::size_t total_memory_usage;

    std::map<hash_type, backtrace_symbols_type> backtrace_symbols;
} memcheck;

/**********************************************************************************************************************
 * Helper functions
 *********************************************************************************************************************/
static hash_type getHashFromBacktrace(const pthread_t tid)
{
    static const size_t N=256;
    void *stack[N];
    int n = backtrace(stack, N);
    assert(n < N-1);
    stack[n] = (void*)tid;
    hash_type hash = boost::hash_range(stack, stack + n + 1);

    // cache demangled backtrace
    RTT::os::MutexLock lock(memcheck.mutex);
    if (memcheck.backtrace_symbols.find(hash) == memcheck.backtrace_symbols.end()) {
        char **mangled_symbols = backtrace_symbols(stack, n);
        memcheck.backtrace_symbols[hash].reserve(n);
        int status;
        // omit first two entries
        for (int i = 2; i < n; ++i) {
            std::string mangled = mangled_symbols[i];
            if (mangled.find('(')  != std::string::npos) mangled = mangled.substr(mangled.find('(') + 1);
            if (mangled.rfind(')') != std::string::npos) mangled = mangled.substr(0, mangled.rfind(')'));
            if (mangled.rfind('+') != std::string::npos) mangled = mangled.substr(0, mangled.rfind('+'));
            char *demangled = abi::__cxa_demangle(mangled.c_str(), 0, 0, &status);
            std::ostringstream ss;
            if (demangled) {
                if (strlen(demangled) <= 200) {
                    ss << std::left << std::setw(200) << demangled << "    [" << stack[i] << "]";
                } else {
                    demangled[200] = '\0';
                    ss << std::left << std::setw(200) << demangled << "... [" << stack[i] << "]";
                }
                ::free(demangled);
            } else {
                ss << mangled_symbols[i];
            }
            memcheck.backtrace_symbols[hash].push_back(ss.str());
        }
        ::free(mangled_symbols);
    }

    return hash;
}

template <typename Stream>
static Stream &logBacktrace(Stream &stream, const backtrace_symbols_type &symbols, const std::string &prefix = "[oro_allocator_memcheck] - ")
{
    const std::ios::fmtflags flags = stream.flags();
    for(std::size_t i = 0; i < symbols.size(); ++i) {
        stream << prefix << "#" << std::setfill(stream.widen(' ')) << std::left << std::setw(3) << i << " " << symbols[i] << std::endl;
    }
    stream.flags(flags);
    return stream;
}

/**********************************************************************************************************************
 * Whenever someone allocates memory...
 *********************************************************************************************************************/
void oro_allocator_memcheck_allocate(void *p, std::size_t n)
{
    if (!memcheck_enabled) return;

    bpt::ptime  now;
    std::string now_s;
    getNow(now, now_s);

    const pthread_t tid = pthread_self();

    hash_type allocator_hash = getHashFromBacktrace(tid);
    RTT::os::MutexLock lock(memcheck.mutex);

    // some basic asserts
    assert(memcheck.allocations_by_pointer.find(p) == memcheck.allocations_by_pointer.end());
//    assert(memcheck.allocator_by_pointer.find(p) == memcheck.allocator_by_pointer.end());
//    assert(memcheck.allocation_size_by_pointer.find(p) == memcheck.allocation_size_by_pointer.end());
//    assert(memcheck.allocation_time_by_pointer.find(p) == memcheck.allocation_time_by_pointer.end());

    // update bookkeeping
    memcheck.nr_of_allocations_by_allocator[allocator_hash]++;
    memcheck.allocated_memory_by_allocator[allocator_hash] += n;
    memcheck.allocations_by_pointer[p] = AllocData(getpid(), pthread_self(), allocator_hash, n, now);
//    memcheck.allocator_by_pointer[p] = allocator_hash;
//    memcheck.allocation_size_by_pointer[p] = n;
//    memcheck.allocation_time_by_pointer[p] = bpt::microsec_clock::local_time();
    memcheck.total_nr_of_allocations++;
    memcheck.total_memory_usage += n;

    // log allocation
    if (memcheck_debug_stream) {
        *memcheck_debug_stream << now_s << ' ' << ((unsigned long int)tid) << ' ' << allocator_hash  << " [oro_allocator_memcheck] Reserved " << n << " bytes at address " << p << ":" << std::endl;
        logBacktrace(*memcheck_debug_stream, memcheck.backtrace_symbols[allocator_hash]) << std::endl;
    }

    // check for massive memory usage by a single allocator (identified by the hash of its backtrace)
    if (memcheck.allocated_memory_by_allocator[allocator_hash] > memcheck_memory_usage_by_allocator_warning_limit) {
        if (memcheck_error_stream) {
            *memcheck_error_stream << now_s << ' ' << ((unsigned long int)tid) << ' ' << allocator_hash << " [oro_allocator_memcheck] [WARNING] New " << n << " byte allocation by the following code path, for a cumulative " << memcheck.allocated_memory_by_allocator[allocator_hash] << " bytes, exceeds warning limit of " << memcheck_memory_usage_by_allocator_warning_limit << " bytes" << std::endl;
            logBacktrace(*memcheck_error_stream, memcheck.backtrace_symbols[allocator_hash]) << std::endl;
        }
    }
    if (memcheck.nr_of_allocations_by_allocator[allocator_hash] > memcheck_nr_of_allocations_by_allocator_warning_limit) {
        if (memcheck_error_stream) {
            *memcheck_error_stream << now_s << ' ' << ((unsigned long int)tid) << ' ' << allocator_hash << " [oro_allocator_memcheck] [WARNING] The number of allocations by the following code path exceeds warning limit: " << memcheck.nr_of_allocations_by_allocator[allocator_hash] << " (> " << memcheck_nr_of_allocations_by_allocator_warning_limit << ")" << std::endl;
            logBacktrace(*memcheck_error_stream, memcheck.backtrace_symbols[allocator_hash]) << std::endl;
        }
    }

    // TODO: add all kind of statistics
}

/**********************************************************************************************************************
 * Whenever someone deallocates memory...
 *********************************************************************************************************************/
void oro_allocator_memcheck_deallocate(void *p, std::size_t n)
{
    if (!memcheck_enabled) return;

    bpt::ptime  now;
    std::string now_s;
    getNow(now, now_s);

    const pthread_t tid = pthread_self();

    hash_type deallocator_hash = getHashFromBacktrace(tid);
    RTT::os::MutexLock lock(memcheck.mutex);

    // check that the memory at p was reserved before
    if (memcheck.allocations_by_pointer.find(p) == memcheck.allocations_by_pointer.end()) {
//    if (memcheck.allocator_by_pointer.find(p) == memcheck.allocator_by_pointer.end()) {
        if (memcheck.deallocator_by_pointer.find(p) == memcheck.deallocator_by_pointer.end()) {
            *memcheck_error_stream << now_s << ' ' << ((unsigned long int)tid) << ' ' << deallocator_hash << " [oro_allocator_memcheck] [ERROR] Freed " << n << " bytes at address " << p << ", but this block was never reserved with oro_rt_malloc() before!" << std::endl;
            logBacktrace(*memcheck_error_stream, memcheck.backtrace_symbols[deallocator_hash]) << std::endl;
        } else {
            *memcheck_error_stream << now_s << ' ' << ((unsigned long int)tid) << ' ' << deallocator_hash << " [oro_allocator_memcheck] [ERROR] Freed " << n << " bytes at address " << p << ", but this block was already freed before!" << std::endl;
            logBacktrace(*memcheck_error_stream, memcheck.backtrace_symbols[deallocator_hash]) << std::endl;
            *memcheck_error_stream << now_s << ' ' << ((unsigned long int)tid) << ' ' << deallocator_hash << " [oro_allocator_memcheck] This is where it was already freed:" << std::endl;
            logBacktrace(*memcheck_error_stream, memcheck.backtrace_symbols[memcheck.deallocator_by_pointer[p]]) << std::endl;
        }
        return;
    }

    // some basic asserts
    assert(memcheck.allocations_by_pointer.find(p) != memcheck.allocations_by_pointer.end());
//    assert(memcheck.allocator_by_pointer.find(p) != memcheck.allocator_by_pointer.end());
//    assert(memcheck.allocation_size_by_pointer.find(p) != memcheck.allocation_size_by_pointer.end());
//    assert(memcheck.allocation_time_by_pointer.find(p) != memcheck.allocation_time_by_pointer.end());
    assert(memcheck.total_memory_usage >= n);

    // who allocated this before?
    hash_type allocator_hash = memcheck.allocations_by_pointer.at(p).allocator;
//    hash_type allocator_hash = memcheck.allocator_by_pointer.at(p);

    // some more asserts
    assert(memcheck.nr_of_allocations_by_allocator[allocator_hash] > 0);
    assert((n == 0) || (memcheck.allocations_by_pointer[p].size == n));
//    assert(n == 0 || memcheck.allocation_size_by_pointer[p] == n);
    assert(memcheck.allocated_memory_by_allocator[allocator_hash] >= memcheck.allocations_by_pointer[p].size);
//    assert(memcheck.allocated_memory_by_allocator[allocator_hash] >= memcheck.allocation_size_by_pointer[p]);
    assert(memcheck.allocations_by_pointer[p].time <= bpt::microsec_clock::local_time());
//    assert(memcheck.allocation_time_by_pointer[p] <= bpt::microsec_clock::local_time());

    // log deallocation
    if (memcheck_debug_stream) {
        *memcheck_debug_stream << now_s << ' ' << ((unsigned long int)tid) << ' ' << deallocator_hash <<  " [oro_allocator_memcheck] Freed " << n << " bytes at address " << p << ":" << std::endl;
        logBacktrace(*memcheck_debug_stream, memcheck.backtrace_symbols[deallocator_hash]) << std::endl;
    }

    // update bookkeeping
    memcheck.nr_of_allocations_by_allocator[allocator_hash]--;
    memcheck.allocated_memory_by_allocator[allocator_hash] -= n;
    memcheck.allocations_by_pointer.erase(p);
//    memcheck.allocator_by_pointer.erase(p);
    memcheck.deallocator_by_pointer[p] = deallocator_hash;
//    memcheck.allocation_size_by_pointer.erase(p);
//    memcheck.allocation_time_by_pointer.erase(p);
    memcheck.total_nr_of_allocations--;
    memcheck.total_memory_usage -= n;

    // TODO: add checks here...
}

    /**********************************************************************************************************************
     * Whenever someone deallocates memory...
     *********************************************************************************************************************/
void oro_allocator_memcheck_logState(std::ostream& sss)
{
    bpt::ptime  now;
    std::string now_s;
    getNow(now, now_s);

    RTT::os::MutexLock lock(memcheck.mutex);
    sss << "# timestamp,pid,pthreadid,allocator_hash,pointer,size" << std::endl
        << "# Log at " << now_s << std::endl
        << "# Outstanding allocs=" << memcheck.total_nr_of_allocations << " for " << memcheck.total_memory_usage << " bytes" << std::endl
        << "# Count pointers=" << memcheck.allocations_by_pointer.size() << std::endl;

    std::map<pointer, AllocData>::const_iterator    iter;
    const std::ios::fmtflags flags = sss.flags();
    for (iter= memcheck.allocations_by_pointer.begin();
         iter!=memcheck.allocations_by_pointer.end();
        ++iter)
    {
        const pointer p = iter->first;
        const AllocData& a = iter->second;

        std::string t_s;
        formatTime(a.time, t_s);
        sss << t_s << ','
            << std::left
            << a.pid<< ','
            << std::hex
            << "0x" << a.pthreadid << ','
            << "0x" << a.allocator << ','
            << p << ','     // pointer automatically adds "0x" prefix
            << std::dec
            << a.size
            << std::endl;
    }
    sss.flags(flags);
}

}} // namespace
