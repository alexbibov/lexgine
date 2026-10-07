#include "engine/core/concurrency/thread_affinity.h"

#include "render_command_pipe.h"


namespace lexgine::core {

bool RenderCommandPipe::enqueue(RenderCommand command)
{
    std::lock_guard lock{ m_mutex };
    if (m_pending_commands.size() >= c_max_pending_commands) return false;

    m_pending_commands.push_back(std::move(command));
    return true;
}

void RenderCommandPipe::drain()
{
    concurrency::assertOnMasterWorkerThread();

    // the commands run outside the lock, so that one of them may queue another without deadlocking
    std::vector<RenderCommand> commands_to_run;
    {
        std::lock_guard lock{ m_mutex };
        commands_to_run.swap(m_pending_commands);
    }

    for (RenderCommand const& command : commands_to_run)
    {
        command();
    }
}

}
