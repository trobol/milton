// Copyright (c) 2015-2016 Sergio Gonzalez. All rights reserved.
// License: https://github.com/serge-rgb/milton#license


#if defined(__cplusplus)
extern "C" {
#endif

#include "platform.h"

#include "common.h"
#include "memory.h"
#include "milton_configuration.h"
#include "utils.h"


// The returns value mean different things, but other than that, we're ok
#ifdef _MSC_VER
#define snprintf sprintf_s
#endif

#if MILTON_DEBUG
#define HEAP_BEGIN_ADDRESS ((LPVOID)(1<<18))  // Location to begin allocations
#else
#define HEAP_BEGIN_ADDRESS NULL
#endif

void*   platform_allocate_bounded_memory(size_t size)
{
#if MILTON_DEBUG
    static b32 once_check = false;
    if ( once_check )
    {
        INVALID_CODE_PATH;
    }
    once_check = true;
#endif
    return VirtualAlloc(HEAP_BEGIN_ADDRESS,
                        (size),
                        MEM_COMMIT | MEM_RESERVE,
                        PAGE_READWRITE);

}

void platform_deallocate_internal(void* pointer)
{
    VirtualFree(pointer, 0, MEM_RELEASE);
}

void win32_log(char *format, ...)
{
    char message[ 1024 ];

    int num_bytes_written = 0;

    va_list args;

    assert ( format );

    va_start( args, format );

    num_bytes_written = _vsnprintf(message, sizeof( message ) - 1, format, args);

    if ( num_bytes_written > 0 )
    {
        OutputDebugStringA( message );
    }

    va_end( args );
}

void milton_fatal(char* message)
{
    milton_log("*** [FATAL] ***: \n\t");
    puts(message);
    exit(EXIT_FAILURE);
}

void milton_die_gracefully(char* message)
{
    platform_dialog(message, "Fatal Error");
    exit(EXIT_FAILURE);
}



static char* win32_filter_strings_image =
    "PNG file\0" "*.png\0"
    "JPEG file\0" "*.jpg\0"
    "\0";

static char* win32_filter_strings_milton =
    "MLT file\0" "*.mlt\0"
    "\0";

void win32_set_OFN_filter(OPENFILENAMEA* ofn, FileKind kind)
{
#pragma warning(push)
#pragma warning(disable:4061)
    switch(kind)
    {
    case FileKind_IMAGE:
        {
        ofn->lpstrFilter = (LPCSTR)win32_filter_strings_image;
        ofn->lpstrDefExt = "jpg";
        } break;
    case FileKind_MILTON_CANVAS:
        {
        ofn->lpstrFilter = (LPCSTR)win32_filter_strings_milton;
        ofn->lpstrDefExt = "mlt";
        } break;
    default:
        {
        INVALID_CODE_PATH;
    }
    }
#pragma warning(pop)
}

char* platform_save_dialog(FileKind kind)
{
    cursor_show();
    char* save_filename = (char*)mlt_calloc(MAX_PATH, sizeof(char));

    OPENFILENAMEA ofn = {0};

    ofn.lStructSize = sizeof(OPENFILENAME);
    //ofn.hInstance;
    win32_set_OFN_filter(&ofn, kind);
    ofn.lpstrFile = save_filename;
    ofn.nMaxFile = MAX_PATH;
    /* ofn.lpstrInitialDir; */
    /* ofn.lpstrTitle; */
    //ofn.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
    ofn.Flags = OFN_OVERWRITEPROMPT;
    /* ofn.nFileOffset; */
    /* ofn.nFileExtension; */
    //ofn.lpstrDefExt = "jpg";
    /* ofn.lCustData; */
    /* ofn.lpfnHook; */
    /* ofn.lpTemplateName; */

    b32 ok = GetSaveFileNameA(&ofn);

    if (!ok)
    {
        mlt_free(save_filename);
        return NULL;
    }
    return save_filename;
}

char* platform_open_dialog(FileKind kind)
{
    cursor_show();
    OPENFILENAMEA ofn = {0};

    char* fname = (char*)mlt_calloc(MAX_PATH, sizeof(*fname));

    ofn.lStructSize = sizeof(OPENFILENAME);
    win32_set_OFN_filter(&ofn, kind);
    ofn.lpstrFile = fname;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST;

    b32 ok = GetOpenFileNameA(&ofn);
    if (ok == false)
    {
        free(fname);
        milton_log("[ERROR] could not open file! Error is %d\n", CommDlgExtendedError());
        return NULL;
    }
    return fname;
}

b32 platform_dialog_yesno(char* info, char* title)
{
    cursor_show();
    i32 yes = MessageBoxA(NULL, //_In_opt_ HWND    hWnd,
                          (LPCSTR)info, // _In_opt_ LPCTSTR lpText,
                          (LPCSTR)title,// _In_opt_ LPCTSTR lpCaption,
                          MB_YESNO//_In_     UINT    uType
                         );
    return yes == IDYES;
}

void platform_dialog(char* info, char* title)
{
    cursor_show();
    MessageBoxA( NULL, //_In_opt_ HWND    hWnd,
                 (LPCSTR)info, // _In_opt_ LPCTSTR lpText,
                 (LPCSTR)title,// _In_opt_ LPCTSTR lpCaption,
                 MB_OK//_In_     UINT    uType
               );
}

void platform_fname_at_exe(char* fname, size_t len)
{
    char* base = SDL_GetBasePath();  // SDL returns utf8 path!
    char* tmp = (char*)mlt_calloc(1, len);
    strncpy(tmp, fname, len);
    fname[0] = '\0';
    strncat(fname, base, len);
    size_t pathlen = strlen(fname);
    strncat(fname + pathlen, tmp, len-pathlen);
    mlt_free(tmp) ;
}

b32 platform_delete_file_at_config(char* fname, int error_tolerance)
{
    b32 ok = true;
    char* full = (char*)mlt_calloc(MAX_PATH, sizeof(*full));
    strncpy(full, fname, MAX_PATH);
    platform_fname_at_config(full, MAX_PATH);
    int r = DeleteFileA(full);
    if (r == 0)
    {
        ok = false;

        int err = (int)GetLastError();
        if ( (error_tolerance&DeleteErrorTolerance_OK_NOT_EXIST) &&
             err == ERROR_FILE_NOT_FOUND)
        {
            ok = true;
        }
    }
    mlt_free(full);
    return ok;
}

void win32_print_error(int err)
{
#pragma warning (push,0)
    LPTSTR error_text = NULL;

    FormatMessage(
                  // use system message tables to retrieve error text
                  FORMAT_MESSAGE_FROM_SYSTEM
                  // allocate buffer on local heap for error text
                  |FORMAT_MESSAGE_ALLOCATE_BUFFER
                  // Important! will fail otherwise, since we're not
                  // (and CANNOT) pass insertion parameters
                  |FORMAT_MESSAGE_IGNORE_INSERTS,
                  NULL,    // unused with FORMAT_MESSAGE_FROM_SYSTEM
                  err,
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  (LPTSTR)&error_text,  // output
                  0, // minimum size for output buffer
                  NULL);   // arguments - see note

    if ( error_text )
    {
        milton_log(error_text);
        LocalFree(error_text);
    }
    milton_log("- %d\n", err);
#pragma warning (pop)
}

b32 platform_move_file(char* src, char* dest)
{
    b32 ok = MoveFileExA(src, dest, MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED);
    //b32 ok = MoveFileExA(src, dest, MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH);
    if (!ok)
    {
        int err = (int)GetLastError();
        win32_print_error(err);
    }
    return ok;
}

void platform_fname_at_config(char* fname, size_t len)
{
    char* base = SDL_GetPrefPath("MiltonPaint", "data");
    char* tmp = (char*)mlt_calloc(1, len);
    strncpy(tmp, fname, len);
    fname[0] = '\0';
    strncat(fname, base, len);
    size_t pathlen = strlen(fname);
    strncat(fname + pathlen, tmp, len-pathlen);
    mlt_free(tmp) ;
}

void platform_open_link(char* link)
{
    ShellExecute(NULL, "open", link, NULL, NULL, SW_SHOWNORMAL);
}

WallTime platform_get_walltime()
{
    WallTime wt = {0};
    {
        SYSTEMTIME ST;
        GetLocalTime(&ST);
        wt.h = ST.wHour;
        wt.m = ST.wMinute;
        wt.s = ST.wSecond;
        wt.ms = ST.wMilliseconds;
    }
    return wt;
}

u64 perf_counter()
{
    u64 time = 0;
    LARGE_INTEGER li = {0};
    // On XP and higher, this function always succeeds.
    QueryPerformanceCounter(&li);

    time = (u64)li.QuadPart;

    return time;
}

float perf_count_to_sec(u64 counter)
{
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    float sec = (float)counter / (float)freq.QuadPart;
    return sec;
}

int CALLBACK WinMain(
        HINSTANCE hInstance,
        HINSTANCE hPrevInstance,
        LPSTR lpCmdLine,
        int nCmdShow
        )
{
    milton_main();
}

#if defined(__cplusplus)
}
#endif