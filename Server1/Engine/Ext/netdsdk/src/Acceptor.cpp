#ifndef _ACCEPTOR_CPP_
#define _ACCEPTOR_CPP_

#include "Debug.h"
#include "Acceptor.h"

template<class SVC_HANDLER>
Acceptor<SVC_HANDLER>::Acceptor (Reactor *reactor/* = 0*/)
{
    this->reactor (reactor);
}
template<class SVC_HANDLER>
Acceptor<SVC_HANDLER>::~Acceptor ()
{
    this->handle_close ();
}
template<class SVC_HANDLER>
NDK_HANDLE Acceptor<SVC_HANDLER>::handle () const
{
    return this->peer_acceptor_.handle ();
}
template<class SVC_HANDLER>
int Acceptor<SVC_HANDLER>::open (const InetAddr &local_addr, 
	Reactor *reactor/* = Reactor::instance ()*/,
	int reuse_addr/* = 1*/,
	size_t rcvbuf_size/* = 0*/)
{
    if (reactor == 0)
	return -1;
    if (this->peer_acceptor_.open (local_addr, 
		reuse_addr,
		rcvbuf_size) == -1)
	return -1;

    // Set the peer acceptor's handle into non-blocking mode.  This is a
    // safe-guard against the race condition that can otherwise occur
    // between the time when <select> indicates that a passive-mode
    // socket handle is "ready" and when we call <accept>.  During this
    // interval, the client can shutdown the connection, in which case,
    // the <accept> call can hang!
    NDK::set_non_block_mode (this->peer_acceptor_.handle ());

    int result = reactor->register_handler (this, EventHandler::ACCEPT_MASK);
    if (result != -1)
	this->reactor (reactor);
    else
	this->peer_acceptor_.close ();
    return result;
}
template<class SVC_HANDLER>
int Acceptor<SVC_HANDLER>::close ()
{
    return this->handle_close ();
}
template<class SVC_HANDLER>
int Acceptor<SVC_HANDLER>::handle_close (NDK_HANDLE handle/* = NDK_INVALID_HANDLE*/,
	ReactorMask/* = EventHandler::ALL_EVENTS_MASK*/)
{
    if (this->reactor () != 0)
    {
	this->reactor ()->remove_handler (this->handle (),
		EventHandler::ACCEPT_MASK | EventHandler::DONT_CALL);
	this->reactor (0);
	// Shut down the listen socket to recycle the handles.
	if (this->peer_acceptor_.close () == -1)
	    return -1;
    }
    return 0;
}
template<class SVC_HANDLER>
int Acceptor<SVC_HANDLER>::make_svc_handler (SVC_HANDLER *&sh)
{
    if (sh == 0)
    {
	sh = new SVC_HANDLER;
	if (sh == 0)
	    return -1;
    }
    // Set the reactor of the newly created <SVC_HANDLER> to the same
    // reactor that this <Acceptor> is using.
    sh->reactor (this->reactor ());
    return 0;
}
template<class SVC_HANDLER>
int Acceptor<SVC_HANDLER>::accept_svc_handler (SVC_HANDLER *svc_handler)
{
    if (this->peer_acceptor_.accept (svc_handler->peer (),
		0, // remote addr
		0  // timeout 
		) == -1)
    {
	// Close down handler to avoid memory leaks.
	svc_handler->close ();
	return -1;
    }
    return 0;
}
template<class SVC_HANDLER>
int Acceptor<SVC_HANDLER>::activate_svc_handler (SVC_HANDLER *svc_handler)
{
    if (svc_handler->open ((void *) this) == -1)
    {
	svc_handler->close ();
	return -1;
    }
    return 0;
}
template<class SVC_HANDLER>
int Acceptor<SVC_HANDLER>::handle_input (NDK_HANDLE listener)
{
    // Accept connections from clients. Note that a loop allows us to 
    // accept all pending connections without an extra trip through the 
    // Reactor and without having to use non-blocking I/O...
    do
    {
	// Create a service handler, using the appropriate creation
	// strategy
	SVC_HANDLER *svc_handler = 0;
	if (this->make_svc_handler (svc_handler) == -1)
	    return 0;
	// Accept connection into the Svc_Handler.
	else if (this->accept_svc_handler (svc_handler) == -1)
	    return 0;
	// Activate the <svc_handler> using the designated concurrency
	// strategy
	else if (this->activate_svc_handler (svc_handler) == -1)
	    // Note that <activate_svc_handler> closes the <svc_handler>
	    // on failure
	    return 0;
    }
    // Now, check to see if there is another connection pending and
    // break out of the loop if there is none.
    while (NDK::handle_read_ready(listener, &TimeValue::zero) == 1);

    return 0;
}
#endif

