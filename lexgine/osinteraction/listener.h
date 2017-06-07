// Generic message listener

#ifndef LEXGINE_OSINTERACTION_LISTENER_H

#include <functional>


namespace lexgine { namespace osinteraction {

//! OS-agnostic abstract listener object
class AbstractListener
{
public:
    static const int64_t not_supported = static_cast<int64_t>(0xFFFFFFFFFFFFFFFF);

    /*! handles the given message. param0 - param7 are reserved for system- and user- defined data that may accompany the message being handled.
      The function returns system-defined value depending on the incoming message and on the message processing status.
      It can also return special value AbstractListener::not_supported in case if requested message cannot be handled by this listener.
    */
    int64_t handle(uint64_t message, uint64_t param0, uint64_t param1, uint64_t param2, uint64_t param3,
        uint64_t param4, uint64_t param5, uint64_t param6, uint64_t param7) const;

protected:
    //! this function performs actual processing of received message; it must be implemented by derived class
    virtual int64_t process_message(uint64_t message, uint64_t param0, uint64_t param1, uint64_t param2, uint64_t param3,
        uint64_t param4, uint64_t param5, uint64_t param6, uint64_t param7) const = 0;

    virtual bool doesHandle(uint64_t message) const = 0;    //! returns 'true' if the listener is aimed to handle the given message; returns 'false' otherwise
};



template<uint32_t ... messages> class ConcreteListener;

template<> class ConcreteListener<> : public virtual AbstractListener
{
protected:
    bool doesHandle(uint64_t message) const override
    {
        return false;
    }
};


template<uint64_t head_message, uint64_t ... tail_messages>
class ConcreteListener<head_message, tail_messages...> : public ConcreteListener<tail_messages...>
{
protected:
    bool doesHandle(uint64_t message) const override
    {
        return message == head_message || ConcreteListener<tail_messages...>::doesHandle(message);
    }
};


//! This class should be inherited by user-defined types that wish to implement more than one listener
template<typename... ListenerTypes> class Listeners;

template<typename Listener>
class Listeners<Listener> : public Listener
{
protected:
    bool doesHandle(uint64_t message) const override
    {
        return Listener::doesHandle(message);
    }

    virtual int64_t process_message(uint64_t message, uint64_t param0, uint64_t param1, uint64_t param2, uint64_t param3, uint64_t param4,
        uint64_t param5, uint64_t param6, uint64_t param7) const override
    {
        return Listener::process_message(message, param0, param1, param2, param3, param4, param5, param6, param7);
    }
};

template<typename Listener, typename... OtherListeners>
class Listeners<Listener, OtherListeners...> : public Listener, public Listeners<OtherListeners...>
{
protected:
    bool doesHandle(uint64_t message) const override
    {
        return Listener::doesHandle(message) || Listeners<OtherListeners...>::doesHandle(message);
    }

    virtual int64_t process_message(uint64_t message, uint64_t param0, uint64_t param1, uint64_t param2, uint64_t param3, uint64_t param4,
        uint64_t param5, uint64_t param6, uint64_t param7) const override
    {
        int64_t rv = Listener::doesHandle(message) ?
            Listener::process_message(message, param0, param1, param2, param3, param4, param5, param6, param7) :
            Listeners<OtherListeners...>::process_message(message, param0, param1, param2, param3, param4, param5, param6, param7);
        return rv;
    }
};


}}

#define LEXGINE_OSINTERACTION_LISTENER_H
#endif