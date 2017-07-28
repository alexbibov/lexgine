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




void MainGlobalsBuilder::defineGlobalSettings(GlobalSettings const& global_settings)
{
    m_global_settings = global_settings;
    m_thread_logs.resize(global_settings.getNumberOfWorkers());
}

void MainGlobalsBuilder::registerThreadLog(uint8_t worker_id, std::ostream* logging_output_stream)
{
    assert(m_global_settings.isValid());
    assert(worker_id < static_cast<GlobalSettings&>(m_global_settings).getNumberOfWorkers());

    m_thread_logs[worker_id] = logging_output_stream;
}

void MainGlobalsBuilder::registerMainLog(std::ostream* logging_output_stream)
{
    m_main_log = logging_output_stream;
}

Globals MainGlobalsBuilder::build()
{
    Globals rv;
    GlobalSettings& global_settings = static_cast<GlobalSettings&>(m_global_settings);
    rv.put(&global_settings);
    rv.put(&m_thread_logs);
    rv.put(&m_main_log);

    return rv;
}
