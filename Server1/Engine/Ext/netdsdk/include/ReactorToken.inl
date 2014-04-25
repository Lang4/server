inline
ReactorToken::ReactorToken (ReactorImpl *r,
	int s_queue/* = Token::FIFO*/)
: Token (s_queue)
, reactor_ (r)
{
}
inline
ReactorToken::~ReactorToken ()
{
}
inline
ReactorImpl *ReactorToken::reactor ()
{
    return this->reactor_;
}
inline
void ReactorToken::reactor (ReactorImpl *r)
{
    this->reactor_ = r;
}

