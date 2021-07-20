

#include "edt_utils.h"

#include  <sys/stat.h>

#include <direct.h>

static LARGE_INTEGER frequency = {0,0};

/**
 * Return the time in seconds since the last call to edt_dtime.
 *
 * Each time edt_dtime is called, it returns the time delta since the last time it was called. Therefore the first time
 * edt_dtime is called in a process the delta may be meaningless. Time is in seconds and fractions of a second.
 *
 * This call uses system time, which is system dependent and may be too coarse for some applications. For more accurate
 * time stamping consider the IRIG option and an external IRIG timestamp source, available with some EDT boards.
 *
 * @RETURN delta time, as a double floating point 
 */
double
edt_dtime()
{
    double  dtime;

    static LARGE_INTEGER starttime;

    LARGE_INTEGER endtime;

    QueryPerformanceCounter(&endtime);
    if (frequency.QuadPart == 0)
        QueryPerformanceFrequency(&frequency);

    dtime = ((double) endtime.QuadPart - (double) starttime.QuadPart)
        / (double) frequency.QuadPart;

    starttime = endtime;
    return (dtime);
}


/**
* Gets the seconds + fractional seconds since the Epoch, OS independent.
* Uses system time; accuracy is OS dependent.
*
* @return the timestamp in seconds; a double value with tenths/hundredths/etc on the right of the decimal point.
*/
double
edt_timestamp()
{
    double  dtime;

    LARGE_INTEGER endtime;

    QueryPerformanceCounter(&endtime);
    if (frequency.QuadPart == 0)
        QueryPerformanceFrequency(&frequency);

    dtime = ((double) endtime.QuadPart)
        / (double) frequency.QuadPart
        ;

    return (dtime);
}



/**
* Causes the process to sleep for the specified number of milliseconds.
*
* @param msecs  The number of milliseconds for the process to sleep.
*
* @return 0 on success, -1 on failure. If an error occurs, call
* #edt_perror to get the system error message. 
*/
void
edt_msleep(int msecs)
{

    Sleep(msecs);

}


/**
* causes process to sleep for specified number of microseconds
*
* @param usecs: number of microseconds for process to sleep
*
* @return 0 on success; -1 on error
*/
void
edt_usleep(int usecs)
{
    Sleep(usecs/1000);
}

void
edt_usec_busywait(u_int usec)
{
    double t = 0.0, f_usec = usec / 1000000;

    edt_dtime();    	   /* edt_dtime() returns a double containing the delta time since last called. */
    while (t < f_usec)
        t += edt_dtime();  /* spin until the sum of the deltas equals the input. */
}


HANDLE edt_open_datafile(const char *path,
                         const char *szFileName,
                         u_char writing,
                         u_char direct,
                         u_char truncate)
{

    int ok = 0;

    HANDLE pFile;

    int rwmode;

    int createmode;

    int flags = 0;

    char name[256];

    if (path)
        sprintf(name, "%s/%s", path, szFileName);
    else
        sprintf(name, "%s",szFileName);

    createmode = CREATE_ALWAYS;

    if (direct) 
        flags = FILE_FLAG_NO_BUFFERING;

    if (writing)
    {
        rwmode = GENERIC_WRITE | GENERIC_READ;
        createmode = CREATE_ALWAYS;
        if (truncate)
        {
            if (access(name, 0) == 0)
                createmode = TRUNCATE_EXISTING;
        }
    }
    else
    {
        rwmode = GENERIC_WRITE | GENERIC_READ;
        createmode = OPEN_EXISTING;
    }

    pFile = CreateFile(name,
        rwmode,
        FILE_SHARE_READ,
        NULL,
        createmode,
        flags,
        NULL
        );


    ok = (pFile != NULL_HANDLE);

    if (pFile == NULL_HANDLE)
    {
        int error = GetLastError();

        printf("Error opening %s = %08x\n", name, error);
    }
    return pFile;
}

void edt_close_datafile(HANDLE f)

{
    CloseHandle(f);
}

int edt_write_datafile(HANDLE f, void *p, int bufsize)

{

    unsigned long rc;

    WriteFile(f,
        p,
        bufsize, 
        &rc,
        NULL);

    return rc;

}

int edt_read_datafile(HANDLE f, void *p, int bufsize)

{

    unsigned long rc;

    ReadFile(f,
        p,
        bufsize, 
        &rc,
        NULL);

    if (rc == 0)
    {
        int error = GetLastError();
        printf("read_data_file: error = %d\n",error);
    }
    return rc;

}

void *edt_alloc_aligned(int size)

{
    return (unsigned char *)
        VirtualAlloc(NULL, size, MEM_COMMIT, PAGE_READWRITE);
}

void edt_free_aligned(void *p)
{
    VirtualFree(p,0,MEM_RELEASE);
}


#include <conio.h>

u_char edt_wait_for_console_input(char *line,
                                  const int timeout,  
                                  const int maxlen)

{

    int t = 0;

    while (t < timeout && !kbhit())
    {
        edt_msleep(10);
        t += 10;
    }

    if (t < timeout)
    {
        fgets(line, maxlen, stdin);
        if (strlen(line))
            if (line[strlen(line)-1] == '\n')
                line[strlen(line)-1] = 0;

        return TRUE;
    }
    else
        return FALSE;

}
#include <sys/timeb.h>

#include <time.h>

int edt_get_datestr(char *s, int maxlen)

{
    time_t tm;

    time(&tm);

    strncpy(s,ctime(&tm),maxlen);

    if (s[strlen(s)-1] == '\n')
        s[strlen(s)-1] = 0;

    return 0;
}

uint64_t
edt_get_file_position(HANDLE f)

{
    uint64_t rc;

    LARGE_INTEGER set;

    set.QuadPart = 0;

    set.LowPart = SetFilePointer(f, set.LowPart, &set.HighPart, FILE_CURRENT);

    rc = set.QuadPart;

    return rc;
}

uint64_t
edt_get_file_size(HANDLE f)

{
    LARGE_INTEGER pos;

    pos.LowPart = GetFileSize(f, (LPDWORD)&pos.HighPart);

    return pos.QuadPart;

}

uint64_t edt_file_seek(HANDLE f, uint64_t to)

{
    uint64_t rc;

    LARGE_INTEGER set;

    set.QuadPart = to;

    set.LowPart = SetFilePointer(f, set.LowPart, &set.HighPart, FILE_BEGIN);

    rc = set.QuadPart;

    return rc;
}

/**
* Convenience routine to allocate memory in a system-independent way.
* The buffer returned is page aligned. Uses VirtualAlloc on Windows NT
* systems, valloc on UNIX-based systems.
*
* @param size:	number of bytes of memory to allocate.
*
* @return The address of the allocated memory, or NULL on error. If
* NULL, use #edt_perror to print the error.
*/

unsigned char *
edt_alloc(int size)
{
    unsigned char *ret;

    ret = (unsigned char *)
        VirtualAlloc(NULL, size, MEM_COMMIT, PAGE_READWRITE);

    return (ret);
}

/**
* Convenience routine to free the memory allocated with #edt_alloc.
*
* @param ptr   Address of memory buffer to free.
*
* @return 0 if sucessful, -1 if unsuccessful.
*/

void
edt_free(uchar_t *ptr)
{

    VirtualFree(ptr, 0, MEM_RELEASE);

}

static double elapsed_start = 0.0;

/** 
* Like #edt_dtime, but doesn't reset start time unless requested.
* @param reset true if start time should be reset. 
*/

double 
edt_elapsed(u_char reset)
{

    if (reset || elapsed_start == 0.0)
    {
        elapsed_start = edt_timestamp();
    }

    return edt_timestamp() - elapsed_start;
}

/*
* below are utility routines, intended primarily for EDT library
* internal use but available for application use if desired
*/

/**
* fwd_to_back: for use by #edt_correct_slashes
*/
void
edt_fwd_to_back(char *str)
{
    char   *p = str;

    while (*p != '\0')
    {
        if (*p == '/')
            *p = '\\';
        ++p;
    }
}


/**
* back_to_fwd: for use by #edt_correct_slashes
*/
void
edt_back_to_fwd(char *str)
{
    char   *p = str;

    while (*p != '\0')
    {
        if (*p == '\\')
            *p = '/';
        ++p;
    }
}



/**
* change the slashes from forward to back or vice versa depending
* on whether it's Wind*ws or Un*x
*/
void
edt_correct_slashes(char *str)
{
    edt_fwd_to_back(str);
}

/**
* Determines file access, independent of operating system. 
*
* This a convenience routine that maps to access() on Unix/Linux
* systems and _access() on Windows systems.
*
* @param fname     path name of file to check access permissions
* @param perm      permission flag(s) to test for. See documentation
* for access() (Unix/Linux) or _access() (Windows) for valid values.
*
* \Example
* @code
* if(edt_access("file.ras", F_OK))
*     printf("Warning: overwriting file %s\n", "file.ras");
* @endcode
*
* @return 0 on success, -1 on failure
*/

int
edt_access(char *fname, int perm)
{
    int     ret;

    edt_correct_slashes(fname);
    ret = _access(fname, perm);

    return ret;
}


/* emulate opendir/readdir */
/**
* @fn void edt_closedir(DIRHANDLE dirphandle)
*
* Cross platform function to close a directory handle. 
*/

/**
* @fn DIRHANDLE edt_opendir(char *dirname)
*
* Cross platform function to open a directory for reading.
* @param dirname the name of the directory to open.
*/

/**
* @fn int edt_readdir(DIRHANDLE dirphandle, char *name)
*
* Cross platform function to read the next directory entry. 
*
* @param dirphandle a handle for the directory, returned by #edt_opendir
*
* @param name a pointer to a string into which the name of the next
* file in the directory will be written.  The string must be allocated
* by the caller to at least 256 bytes.
*
*/

char    Next_name[256];
DWORD   Err = 0;

HANDLE
edt_opendir(const char *dirname)
{
    WIN32_FIND_DATA fd;
    HANDLE  h;
    char    searchname[256];

    /*
    * This fixes a bug where second time through  this function fails
    */
    SetLastError(0);

    sprintf(searchname, "%s\\*", dirname);
    if ((h = FindFirstFile(searchname, &fd)) == INVALID_HANDLE_VALUE)
        return NULL;
    if ((Err = GetLastError()) != ERROR_NO_MORE_FILES)
        strcpy(Next_name, fd.cFileName);

    return h;
}

int
edt_readdir(HANDLE h, char *name)
{

    WIN32_FIND_DATA fd;
    int rc;

    /* get file and check error from previous FindFirstFile or FindNextFile */
    if (Err == ERROR_NO_MORE_FILES)
        return 0;

    strcpy(name, Next_name);

    /* get the next one */
    rc = FindNextFile(h, &fd);
    Err = GetLastError();
    if (rc)
    {
        strcpy(Next_name, fd.cFileName);
    }
    else
        Next_name[0] = 0;

    return 1;
}

void
edt_closedir(HANDLE h)
{
    FindClose(h);
}

/**
* Return the size in bytes of available free space 
* for file systen containing directory name.
*/

int64_t edt_disk_free(const char *name)

{
    ULARGE_INTEGER freebytes;
    ULARGE_INTEGER totalbytes;
    ULARGE_INTEGER totalfree;

    int rc;

    rc = GetDiskFreeSpaceEx(name, &freebytes, &totalbytes, &totalfree);

    if (rc)
        return freebytes.QuadPart;
    else
        return -1;

}

int64_t edt_disk_size(const char *name)

{
    ULARGE_INTEGER freebytes;
    ULARGE_INTEGER totalbytes;
    ULARGE_INTEGER totalfree;

    int rc;

    rc = GetDiskFreeSpaceEx(name, &freebytes, &totalbytes, &totalfree);

    if (rc)
        return totalbytes.QuadPart;
    else
        return -1;

}

int64_t edt_disk_used(const char *name)

{
    ULARGE_INTEGER freebytes;
    ULARGE_INTEGER totalbytes;
    ULARGE_INTEGER totalfree;

    int rc;

    rc = GetDiskFreeSpaceEx(name, &freebytes, &totalbytes, &totalfree);

    if (rc)
        return totalbytes.QuadPart - freebytes.QuadPart;
    else
        return -1;

}

int edt_file_exists(const char *name, int rdonly)

{
    int rc = access(name, (rdonly) ? 4:6);
    return (rc == 0);
}

int edt_mkdir(const char *name, int access)

{
    return _mkdir(name);
}


int edt_mkdir_p(const char *name, int access)

{

    if (edt_file_exists(name, access))
    {
        return 0;
    }
    else
        return _mkdir(name);

#if 0
    stat(snapshotDir.c_str(), &st);
        if (!S_ISDIR(st.st_mode))
            mkdir(snapshotDir.c_str(), S_IRWXU);
#endif

    
}
