#ifndef LEXGINE_CORE_INITIALIZER
#define LEXGINE_CORE_INITIALIZER

#include "globals.h"
#include "global_settings.h"
#include "lexgine/core/misc/log.h"
#include "lexgine/core/dx/d3d12/lexgine_core_dx_d3d12_fwd.h"
#include "lexgine/core/dx/d3d12/task_caches/lexgine_core_dx_d3d12_task_caches_fwd.h"

#include <fstream>
#include <memory>

namespace lexgine { namespace core {

/*! This class implements initialization protocol of the engine
 IMPORTANT NOTE: currently the Initializer class depends on windows API functions to retrieve
 the time zone information for logging. This must be changed in order to support cross-platform compilation
*/
class Initializer
{
public:
    /*! Initializes environment of the engine (e.g. logging, global parameters and so on). Returns 'true' on success and 'false' on failure
    here global_look_up_prefix will be added to ALL look-up directories (settings look-up, shaders look-up, assets look-up and so on);
    settings_lookup_path is used searching for JSON configuration files
    logging_output_path is the path where to output the logging information. Note that this is not a look-up directory, so it is NOT prefixed
    with global_lookup_prefix
    */
    Initializer(
        std::string const& global_lookup_prefix = "",
        std::string const& settings_lookup_path = "",
        std::string const& global_settings_json_file = "global_settings.json",
        std::string const& logging_output_path = "",
        std::string const& log_name = "lexgine_log"
    );

    //! Shuts down the environment
    ~Initializer();

    core::Globals& globals();    //! returns the global parameter object pool. This function must not be exposed to the client and will be removed in the future.



private:

    std::ofstream m_logging_file_stream;
    std::vector<std::ofstream> m_logging_worker_file_streams;
    std::vector<std::ostream*> m_logging_worker_generic_streams;
    std::unique_ptr<GlobalSettings> m_global_settings;
    std::unique_ptr<core::Globals> m_globals;
    std::unique_ptr<dx::d3d12::DxResourceFactory> m_resource_factory;
    std::unique_ptr<dx::d3d12::task_caches::HLSLCompilationTaskCache> m_shader_cache;
    std::unique_ptr<dx::d3d12::task_caches::PSOCompilationTaskCache> m_pso_cache;
    std::unique_ptr<dx::d3d12::task_caches::RootSignatureCompilationTaskCache> m_rs_cache;
};

}}

#endif
