#include "../include/SockConnector.h"
#include "../include/NDK.h"

#include <deque>

class MsgHeader
{
public:
    int msgid;
    int msglen;  // not include header
    MsgHeader ()
    : msgid (-1)
    , msglen (0)
    {}
};
enum
{
    TRANSFER_FILE  = 200,
    ECHO_MSG	   = 202,
    EXIT_CMD	   = 204
};
int main (int argc, char *argv[])
{
    signal (SIGPIPE, SIG_IGN);
    char *usage = "./prog 192.168.1.1 8889 clientnum interval(msec) packnum\n";
    if (argc != 6) 
    {
	printf (usage);
	return -1;
    }
    int clnum = atoi(argv[3]);
    std::deque<SockStream*> client_list;
    for (int i = 0; i < clnum; ++i)
    {
	SockStream *peer = new SockStream;
	SockConnector connector;
	InetAddr addr (atoi(argv[2]), argv[1]);
	if (connector.connect (*peer, addr) == 0)
	    client_list.push_back(peer);
    }
    printf ("connect success : %d, failed : %d\n", 
	    client_list.size(),
	    clnum - client_list.size());
    if (client_list.empty ()) return -1;
    MsgHeader msg;
    int result = 0;
    char buff[1400] = {0};
    iovec iov[2];
    int count = 0;
    int inter = atoi(argv[4]);
    TimeValue tv(0, inter*1000);
    int packnum = atoi(argv[5]);
    std::deque<SockStream*>::iterator pos;
    for (int i = 0; i < packnum; ++i)
    {
	for (pos = client_list.begin(); pos != client_list.end();)
	{
	    msg.msgid = TRANSFER_FILE;
	    msg.msglen = sizeof(buff);
	    iov[0].iov_base = &msg;
	    iov[0].iov_len  = sizeof(msg);
	    iov[1].iov_base = buff;
	    iov[1].iov_len  = msg.msglen;
	    result = (*pos)->sendv (iov, 2);
	    if (result <= 0) 
	    {
		pos = client_list.erase(pos);
		printf ("send failed\n");
	    }else
		++pos;
	}
	NDK::sleep (&tv);
    }
    for (pos = client_list.begin(); pos != client_list.end(); ++pos)
    {
	(*pos)->close ();
    }
    return 0;
}
