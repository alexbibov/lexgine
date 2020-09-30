#ifndef LEXGINE_CORE_ERROR_BEHAVIORAL_H
#define LEXGINE_CORE_ERROR_BEHAVIORAL_H

#include <string>
#include <sstream>
#include <functional>

#include "misc/log.h"


namespace lexgine::core {

//! Implements basic error behavior and logging. Inherited by every entity in the engine.
class ErrorBehavioral
{
public:

    //! Registers new error callback. Error callback is a function, which is called immediately when the object enters
    //! erroneous state
    void registerErrorCallback(std::function<void(std::string const& err_msg)> error_callback);

    //! Adds output stream to the list of streams used by the logging system
    void addLoggingStream(std::ostream& log_stream, std::string const& log_name);

    //! Resets error state of the object to 'false' and returns the previous value of the error state
    bool resetErrorState();

    //! Returns current error state of the object (i.e. 'true' if the object IS in erroneous state; 'false' otherwise)
    bool getErrorState() const;

    //! Returns textual description of the last error occurred
    char const* getErrorString() const;

    virtual ~ErrorBehavioral();


private:
    //!< Error status of the object. Equals 'true' if the object is in an erroneous state; 'false' otherwise
    mutable bool m_error_state;

    //!< String describing the last error occurred in relation with this object
    mutable std::string m_error_string;

    //!< Callback function that is called when the object enters an erroneous state
    std::function<void(std::string const& err_msg)> m_error_callback;


protected:
    //! Puts object into erroneous state setting error description provided via @param error_message and making sure that
    //! the information regarding the error is forwarded to the error callback function and included into the log
    void raiseError(std::string const& error_message) const;

    //! Returns reference to the logging system used by the thread
    misc::Log const& logger() const;



    // Error behavioral objects can only be created by inherited infrastructure

    ErrorBehavioral();
};

}

#endif