inline
SockConnector::SockConnector ()
{
}
inline
SockConnector::~SockConnector ()
{
}
inline
int SockConnector::shared_open (SockStream &new_stream, 
	int protocol_family)
{
    TRACE ("SockConnector");
    if (new_stream.handle () == NDK_INVALID_HANDLE
	    && new_stream.open (SOCK_STREAM, protocol_family) == -1)
	return -1;
    return 0;
}


