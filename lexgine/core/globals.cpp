#include "globals.h"

#include <cassert>

using namespace lexgine::core;



void* Globals::find(misc::HashedString const& hashed_name)
{
    global_object_pool_type::iterator target_entry;
    if ((target_entry = m_global_object_pool.find(hashed_name)) == m_global_object_pool.end()) return nullptr;

    return target_entry->second;
}

void const* Globals::find(misc::HashedString const& hashed_name) const
{
    return const_cast<Globals*>(this)->find(hashed_name);
}

bool Globals::put(misc::HashedString const& hashed_name, void* p_object)
{
    return m_global_object_pool.insert(std::make_pair(hashed_name, p_object)).second;
}




void MainGlobalsBuilder::defineGlobalSettings(GlobalSettings& global_settings)
{
    m_global_settings = &global_settings;
}

void MainGlobalsBuilder::registerWorkerThreadLogs(std::vector<std::ostream*>& worker_threads_logging_output_streams)
{
    assert(m_global_settings);
    uint8_t num_workers = m_global_settings->getNumberOfWorkers();
    assert(worker_threads_logging_output_streams.size() == num_workers);

    m_worker_logs = &worker_threads_logging_output_streams;
}

void MainGlobalsBuilder::registerMainLog(std::ostream& logging_output_stream)
{
    m_main_log = &logging_output_stream;
}

Globals MainGlobalsBuilder::build()
{
    Globals rv;
    rv.put(m_global_settings);
    rv.put(m_worker_logs);
    rv.put(m_main_log);

    return rv;
}
