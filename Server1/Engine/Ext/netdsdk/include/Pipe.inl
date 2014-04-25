inline
NDK_HANDLE Pipe::read_handle ()
{
    return this->handles_[0];
}
inline
NDK_HANDLE Pipe::write_handle ()
{
    return this->handles_[1];
}

