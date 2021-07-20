
#ifndef _LOGBUF_H_
#define _LOGBUF_H_

#include <streambuf>

using namespace std;

class EdtLogStreambuf : public streambuf

{
private:

    streambuf * sbuf;
    streambuf * lbuf;

    bool do_output;
    bool do_logging;

    bool output_state;
    bool logging_state;

public:

    EdtLogStreambuf(ostream *s = NULL, ostream *l = NULL)
    {
        SetMainStreambuf(s);
        SetLogStreambuf(l);
    }
    
    virtual ~EdtLogStreambuf() {}

    virtual void SetMainStreambuf(ostream *s)
    {
        if (s)
            sbuf = (streambuf *) s->rdbuf();
        else

            sbuf = NULL;

        SetOutput(sbuf != NULL);

    }

    virtual void SetLogStreambuf(ostream *l)
    {
        if (l)
            lbuf = (streambuf *) l->rdbuf();
        else
            lbuf = NULL;

        SetLogging(lbuf != NULL);
    }

    virtual void SetLogStreambuf(streambuf * l)

    {
        lbuf = (streambuf *) l;
        SetLogging(lbuf != NULL);
    }

    virtual void SetMainStreambuf(streambuf * l)

    {
        sbuf = (streambuf *) l;
        SetOutput(sbuf != NULL);
    }

    virtual streambuf * LogStreambuf()

    {
        return lbuf;
    }

    virtual streambuf * MainStreambuf()

    {
        return sbuf;
    }

    virtual int sputc(char c)
    {
        if (do_output)
        {
            sbuf->sputc(c);
        }
        if (do_logging)
            lbuf->sputc(c);
           
        if (c == '\n')
             sync();
 
        return c;
    }

    virtual streamsize sputn(const char *s, streamsize n)
    {
        if (do_output)
            sbuf->sputn(s,n);;
        if (do_logging)
        {
             lbuf->sputn(s,n);
        }
        return n;
    }

    virtual streamsize xsputn(const char *s, streamsize n)
    {
        if (do_output)
            sbuf->sputn(s,n);;
        if (do_logging)
        {           
            lbuf->sputn(s,n);   
        }
        return n;
    }

    virtual int overflow(int c)
    {
        if (do_output)
            sbuf->sputc(c);
        if (do_logging)
        {
            lbuf->sputc(c);
            
        }
        return c;
    }

    virtual void SetOutput(const bool state)
    {
        if (state && sbuf)
            do_output = true;
        else
            do_output = false;
        output_state = do_output;
    }

    virtual void SetLogging(const bool state)
    {
        if (state && lbuf)
            do_logging = true;
        else
            do_logging = false;
        logging_state = do_logging;
    }

    bool Outputting() const
    {
        return do_output;
    }

    bool Logging() const
    {
        return do_logging;
    }

    virtual int sync()
    {
        if (do_output)
            sbuf->pubsync();
        if (do_logging)
            lbuf->pubsync();
        reset_temp_flags();
        return 0;
    }

    virtual void reset_temp_flags()
    {
        do_output = output_state;
        do_logging = logging_state;
    }

    virtual void SetTempLogging()
    {
        do_output = false;
    }

    virtual void SetTempOutput()
    {
        do_logging = false;
    }
};


inline ostream &
log_only(ostream & other)
{	// insert newline and flush byte stream
	EdtLogStreambuf * lb = dynamic_cast<EdtLogStreambuf *>(other.rdbuf());
    if (lb)
        lb->SetTempLogging();
    return other;
}

inline ostream &
out_only(ostream & other)
{	// insert newline and flush byte stream
	EdtLogStreambuf * lb = dynamic_cast<EdtLogStreambuf *>(other.rdbuf());
    if (lb)
        lb->SetTempOutput();
    return other;
}
#endif
