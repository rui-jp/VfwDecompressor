#ifdef _DEBUG
#pragma comment( lib, "strmbasd" )
#else
#pragma comment( lib, "strmbase" )
#endif

#pragma comment( lib, "strmiids" )
#pragma comment( lib, "winmm" )
#pragma comment( lib, "vfw32" )

#include <streams.h>
#include <initguid.h>
#include "VfwDecompressor.h"

static WCHAR g_wszName[] = L"VFW Decompressor";

static const AMOVIESETUP_MEDIATYPE inMediaTypes[] = {
  { &MEDIATYPE_Video, &MEDIASUBTYPE_NULL },
};
static const AMOVIESETUP_MEDIATYPE outMediaTypes[] = {
  { &MEDIATYPE_Video, &MEDIASUBTYPE_RGB32 },
  { &MEDIATYPE_Video, &MEDIASUBTYPE_RGB24 },
};

static const AMOVIESETUP_PIN sudFilterPins[] =
{
    {
        L"",            // Obsolete, not used.
        FALSE,          // Is this pin rendered?
        FALSE,          // Is it an output pin?
        FALSE,          // Can the filter create zero instances?
        FALSE,          // Does the filter create multiple instances?
        &CLSID_NULL,    // Obsolete.
        NULL,           // Obsolete.
        1,              // Number of media types.
        inMediaTypes    // Pointer to media types.
    },
    {
        L"",            // Obsolete, not used.
        FALSE,          // Is this pin rendered?
        TRUE,           // Is it an output pin?
        FALSE,          // Can the filter create zero instances?
        FALSE,          // Does the filter create multiple instances?
        &CLSID_NULL,    // Obsolete.
        NULL,           // Obsolete.
        2,              // Number of media types.
        outMediaTypes   // Pointer to media types.
    }
};

static const AMOVIESETUP_FILTER sudFilterReg =
{
    &CLSID_VfwDecompressor,   // Filter CLSID
    g_wszName,                // String name
    MERIT_NORMAL + 0x100,     // Filter merit
    2,                        // Number of pins
    sudFilterPins             // Pin information
};

CFactoryTemplate g_Templates[] = {
  {
    g_wszName,
    &CLSID_VfwDecompressor,
    VfwDecompressor::CreateInstance,
    NULL,
    &sudFilterReg
  }
};
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

STDAPI DllRegisterServer()
{
  return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
  return AMovieDllRegisterServer2(FALSE);
}

extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

BOOL APIENTRY DllMain(HANDLE hModule, DWORD  dwReason, LPVOID lpReserved)
{
  return DllEntryPoint((HINSTANCE)(hModule), dwReason, lpReserved);
}

