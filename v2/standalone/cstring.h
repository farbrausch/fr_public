
// minimal mockup implementation of MFC/WTL's CString just because.

/*class CString
{
private:
  char *str;

  inline void Alloc(int len)
  {
    delete[] str;
    str = new char[len+1];
  }

  inline void FromCStr(const char *cstr)
  {
    int len = strlen(cstr);
    Alloc(len);
    strcpy_s(str,len+1,cstr);
  }

public:

  inline CString() : str(0) {}
  inline CString(const CString &str) : str(str.str) {}
  inline CString(const char *cstr) { FromCStr(cstr);  }
  ~CString() { delete[] str; }

  inline void Format(const char *fmt, ...)
  {
    va_list argp;
    va_start(argp, fmt);

    char buffer[1024];
    vsprintf_s(buffer,fmt,argp);
    FromCStr(buffer);
    
    va_end(argp);
  }

  inline operator const char* () { return str; }
};*/
