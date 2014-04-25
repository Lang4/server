//========================================================================
/**
 * Author   : cuisw <shaovie@gmail.com>
 * Date     : 2008-03-11 22:30
 */
//========================================================================

#ifndef _ACCEPTOR_H_
#define _ACCEPTOR_H_
#include "Pre.h"

#include "Reactor.h"
#include "InetAddr.h"
#include "SockStream.h"
#include "SockAcceptor.h"
#include "EventHandler.h"

/**
 * @class Acceptor
 *
 * @brief 
 */
template<class SVC_HANDLER>
class Acceptor : public EventHandler
{
public:
    Acceptor (Reactor * = 0);
    virtual ~Acceptor ();

    /**
     * Begin to listening, and register with the specified reactor 
     * for accept events. An acceptor can only listen to one port 
     * at a time, so make sure to close() the acceptor before calling
     * open() again.
     *
     */
    virtual int open (const InetAddr &local_addr,
	    Reactor *reactor = Reactor::instance (),
	    int reuse_addr = 1,
	    size_t rcvbuf_size = 0);
    
    // Returns the listening acceptor's {NDK_HANDLE}.
    virtual NDK_HANDLE handle (void) const;

    // Close down the Acceptor
    virtual int close (void);
protected:
    // = The following three methods define the Acceptor's strategies
    // for creating, accepting, and activating SVC_HANDLER's,
    // respectively

    /**
     * Bridge method for creating a SVC_HANDLER.  The default is to
     * create a new {SVC_HANDLER} if {sh} == 0, else {sh} is unchanged.
     * However, subclasses can override this policy to perform
     * SVC_HANDLER creation in any way that they like (such as creating
     * subclass instances of SVC_HANDLER, using a singleton, dynamically
     * linking the handler, etc.).  Returns -1 on failure, else 0.
     */
    virtual int make_svc_handler (SVC_HANDLER *&sh);

    /**
     * Bridge method for accepting the new connection into the
     * <svc_handler>.
     */
    virtual int accept_svc_handler (SVC_HANDLER *svc_handler);

    /**
     * Bridge method for activating a {svc_handler} with the appropriate
     * concurrency strategy.  The default behavior of this method is to
     * activate the SVC_HANDLER by calling its {open} method (which
     * allows the SVC_HANDLER to define its own concurrency strategy).
     * However, subclasses can override this strategy to do more
     * sophisticated concurrency activations (such as making the
     * SVC_HANDLER as an "active object" via multi-threading).
     */
    virtual int activate_svc_handler (SVC_HANDLER *svc_handler);

    // = Demultiplexing hooks.
    // Perform termination activities when {this} is removed from the
    // {reactor}.
    virtual int handle_close (NDK_HANDLE = NDK_INVALID_HANDLE,
	    ReactorMask = EventHandler::ALL_EVENTS_MASK);

    // Accepts all pending connections from clients, and creates and
    // activates SVC_HANDLERs.
    virtual int handle_input (NDK_HANDLE);
private:
    // Needed to reopen the socket if {accept} fails.
    InetAddr peer_acceptor_addr_;

    // Socke Acceptor
    SockAcceptor peer_acceptor_;
};

#include "Acceptor.cpp"
#include "Post.h"
#endif

