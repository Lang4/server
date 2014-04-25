inline
Trace::Trace (const char *file, const char *func, int line)
:line_ (line)
,file_ (file)
,func_ (func)
{
    Trace::count_++;
    std::fprintf (stderr, "<TRACE>: ");
    for (int i = 0; i < count_ * 2; i++)
	std::fprintf (stderr, " ");
    std::fprintf (stderr, "(%d)<%lu>calling `%s` in file `%s` on line `%d`\n",
	    Trace::count_, Thread::self (),
	    func_, file_, line_);
}
inline
Trace::~Trace ()
{
    std::fprintf (stderr, "<TRACE>: ");
    for (int i = 0; i < count_ * 2; i++)
	std::fprintf (stderr, " ");
    std::fprintf (stderr, "(%d)<%lu>leaving `%s`\n", 
	    Trace::count_, Thread::self (), func_);
    Trace::count_--;
}
