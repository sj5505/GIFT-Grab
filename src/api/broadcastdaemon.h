#pragma once

#include "ivideosource.h"
#ifdef BUILD_PYTHON
#include "gil.h"
#endif
#include "macros.h"
#include <thread>
#include <chrono>
#include <mutex>

namespace gg
{

//!
//! \brief An object of this class serves as a
//! surrogate for implementing the observer pattern
//! (http://www.oodesign.com/observer-pattern.html)
//! for IVideoSource implementors that do not
//! inherently support it due to implementation
//! details related to used external libraries.
//!
//! This class is intended to be used as a visitor
//! (http://www.oodesign.com/visitor-pattern.html) to
//! an IVideoSource object for augmenting that
//! object's functionality.
//!
class BroadcastDaemon
{
protected:
    //!
    //! \brief
    //! \sa the public constructor
    //!
    IVideoSource * _source;

    //!
    //! \brief Flag indicating status to actual
    //! daemon thread
    //!
    bool _running;

    //!
    //! \brief Actual daemon thread
    //! \sa _running
    //! \sa _lock
    //!
    std::thread _thread;

    //!
    //! \brief
    //!
    std::mutex _lock;

public:
    //!
    //! \brief Link given video source with this
    //! object (see class description)
    //! \param source The caller is responsible
    //! for ensuring the life span of the passed
    //! source outlasts this daemon
    //! \throw VideoSourceError if passed source
    //! pointer is null
    //!
    BroadcastDaemon(IVideoSource * source)
        : _source(source)
        , _running(false)
    {
        if (_source == nullptr)
            throw VideoSourceError("Null pointer passed"
                                   " to broadcast daemon");
    }

    //!
    //! \brief
    //!
    virtual ~BroadcastDaemon()
    {
        // nop
    }

public:
    //!
    //! \brief Start broadcasting at specified
    //! frame rate
    //! \param frame_rate in frames per second
    //! \throw VideoSourceError if broadcast
    //! already running, or if invalid frame
    //! rate value passed
    //!
    void start(float frame_rate)
    {
        if (frame_rate <= 0.0)
            throw VideoSourceError("Invalid frame rate");

        float sleep_duration_ms = 1000.0 / frame_rate;
        _running = true;
#ifdef BUILD_PYTHON
        ScopedPythonGILRelease gil_release;
#endif
        _thread = std::thread(&BroadcastDaemon::run,
                              this,
                              frame_rate);
    }

    //!
    //! \brief Stop current broadcast
    //!
    void stop()
    {
        std::lock_guard<std::mutex> lock_guard(_lock);
        _running = false;
        _thread.join();
    }

protected:
    //!
    //! \brief This method is passed to broadcaster
    //! thread
    //! \param sleep_duration_ms
    //!
    void run(float sleep_duration_ms)
    {
        // TODO - colour?
        VideoFrame frame(I420, false);
        while (_running)
        {
            std::lock_guard<std::mutex> lock_guard(_lock);
            if (_source->get_frame(frame))
                _source->notify(frame);
            std::this_thread::sleep_for(
                std::chrono::microseconds(static_cast<int>(1000 * sleep_duration_ms))
            ); // TODO - account for lost time?
        }
    }

private:
    //!
    //! \brief Never use default constructor, as
    //! every object of this class must be linked
    //! to an IVideoSource
    //!
    BroadcastDaemon()
    {
        // nop
    }

private:
    DISALLOW_COPY_AND_ASSIGNMENT(BroadcastDaemon);
};

}
