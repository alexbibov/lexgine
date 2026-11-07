#ifndef LEXGINE_CORE_RENDERING_RENDER_COMMAND_PIPE_H
#define LEXGINE_CORE_RENDERING_RENDER_COMMAND_PIPE_H

#include <cstddef>
#include <functional>
#include <mutex>
#include <vector>


namespace lexgine::core::rendering {

/*! Carries the state changes, which are requested while a frame is being recorded, to the point of the frame
 where they may be applied safely. The commands are queued by whichever thread requests them and are run by
 the thread driving the frames, so nothing mutates the scene in the middle of the recording.
*/
class RenderCommandPipe final
{
public:
    using RenderCommand = std::function<void()>;

public:
    /*! queues @param command to be run at the next drain. Returns 'false' when the pipe is full, in which
     case the command is dropped and the caller is expected to report that
    */
    bool enqueue(RenderCommand command);

    //! runs the queued commands and empties the pipe
    void drain();

private:
    static constexpr size_t c_max_pending_commands = 256;

private:
    std::mutex m_mutex;    //!< guards the queued commands against the threads requesting them
    std::vector<RenderCommand> m_pending_commands;    //!< commands awaiting the next drain
};

}

#endif
