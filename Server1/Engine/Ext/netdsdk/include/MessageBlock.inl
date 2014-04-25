inline
MessageBlock::MessageBlock (const size_t size)
: mb_base_ (0)
, release_base_ (0)
, mb_size_ (size)
, mb_type_ (MB_DATA)
, prev_ (0)
, next_ (0)
{
    mb_base_ = (char*)::malloc((sizeof(char) * size));
    release_base_ = 1;
}
inline
MessageBlock::MessageBlock (const char *pdata, const size_t size)
: mb_base_ (const_cast<char *>(pdata))
, release_base_ (0)
, mb_size_ (size)
, mb_type_ (MB_DATA)
, prev_ (0)
, next_ (0)
{
}
inline
MessageBlock::~MessageBlock ()
{
    //this->release ();
}
inline
char *MessageBlock::base ()
{
    return this->mb_base_;
}
inline
size_t MessageBlock::size ()
{
    return this->mb_size_;
}
inline
size_t MessageBlock::data_type ()
{
    return this->mb_type_;
}
inline
void MessageBlock::data_type (size_t type)
{
    NDK_SET_BITS (this->mb_type_, type);
}
inline
MessageBlock *MessageBlock::next () const
{
    return this->next_;
}
inline
void MessageBlock::next (MessageBlock *mb)
{
    this->next_ = mb;
}
inline
MessageBlock *MessageBlock::prev () const
{
    return this->prev_;
}
inline
void MessageBlock::prev (MessageBlock *mb)
{
    this->prev_ = mb;
}
inline
void MessageBlock::release ()
{
    MessageBlock *mb  = this;
    MessageBlock *tmp = 0;
    for (; mb != 0;)
    {
	tmp = mb;
	mb  = mb->next ();
	tmp->clean ();
	delete tmp;
    }
}
inline
void MessageBlock::clean ()
{
    if (this->mb_base_ && this->release_base_)
    {
	::free (this->mb_base_);
	this->mb_base_ = 0;
	this->mb_size_ = 0;
    }
}

