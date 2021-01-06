#include <windows.h>
#include <streams.h>
#include "VfwDecompressor.h"

CUnknown* VfwDecompressor::CreateInstance(LPUNKNOWN pUnk, HRESULT* pHr)
{
  VfwDecompressor* pFilter = new VfwDecompressor();
  if (pFilter == NULL)
  {
    *pHr = E_OUTOFMEMORY;
  }
  return pFilter;
}

VfwDecompressor::VfwDecompressor() :
  CTransformFilter(NAME("VFW Video Decompress Filter"), 0, CLSID_VfwDecompressor),
  hic_(NULL), beganDecompress_(false)
{
}

VfwDecompressor::~VfwDecompressor()
{
  if (hic_ != NULL)
  {
    ICClose(hic_);
    hic_ = NULL;
  }
}

HRESULT VfwDecompressor::EndOfStream()
{
  if (beganDecompress_) {
    ICDecompressEnd(hic_);
    beganDecompress_ = false;
  }

  return CTransformFilter::EndOfStream();
}

HRESULT VfwDecompressor::Transform(IMediaSample* pSource, IMediaSample* pDest)
{
  HRESULT hr;

  BYTE* pBufferIn;
  BYTE* pBufferOut;

  hr = pSource->GetPointer(&pBufferIn);
  if (FAILED(hr))
  {
    return hr;
  }
  hr = pDest->GetPointer(&pBufferOut);
  if (FAILED(hr))
  {
    return hr;
  }

  CMediaType* inMediaType = &m_pInput->CurrentMediaType();
  CMediaType* outMediaType = &m_pOutput->CurrentMediaType();

  BITMAPINFOHEADER* lpbiIn = &((VIDEOINFOHEADER*)inMediaType->Format())->bmiHeader;
  BITMAPINFOHEADER* lpbiOut = &((VIDEOINFOHEADER*)outMediaType->Format())->bmiHeader;

  if (!beganDecompress_) {
    if (ICERR_OK != ICDecompressBegin(hic_, lpbiIn, lpbiOut))
      return S_FALSE;

    beganDecompress_ = true;
  }

  if (ICERR_OK != ICDecompress(hic_, 0, lpbiIn, pBufferIn, lpbiOut, pBufferOut))
    return S_FALSE;

  LONGLONG mediaStart;
  LONGLONG mediaEnd;
  REFERENCE_TIME timeStart;
  REFERENCE_TIME timeEnd;

  pSource->GetTime(&timeStart, &timeEnd);
  pSource->GetMediaTime(&mediaStart, &mediaEnd);
  pDest->SetTime(&timeStart, &timeEnd);
  pDest->SetMediaTime(&mediaStart, &mediaEnd);

  pDest->SetActualDataLength(lpbiOut->biSizeImage);
  pDest->SetSyncPoint(pSource->IsSyncPoint() == S_OK);
  pDest->SetDiscontinuity(pSource->IsDiscontinuity() == S_OK);

  return S_OK;
}

HRESULT VfwDecompressor::CheckInputType(const CMediaType* mtIn)
{
  if (*mtIn->FormatType() != FORMAT_VideoInfo)
  {
    return VFW_E_TYPE_NOT_ACCEPTED;
  }

  BITMAPINFOHEADER* lpbiIn = &((VIDEOINFOHEADER*)mtIn->Format())->bmiHeader;
  if (lpbiIn->biCompression == BI_RGB)
  {
    return VFW_E_TYPE_NOT_ACCEPTED;
  }

  if (!hic_) {
    hic_ = ICDecompressOpen(ICTYPE_VIDEO, lpbiIn->biCompression, lpbiIn, NULL);
    if (!hic_)
    {
      return VFW_E_TYPE_NOT_ACCEPTED;
    }
  }

  return S_OK;
}

HRESULT VfwDecompressor::CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut)
{
  if (mtOut->majortype != MEDIATYPE_Video)
  {
    return VFW_E_TYPE_NOT_ACCEPTED;
  }

  if (mtOut->formattype != FORMAT_VideoInfo ||
    mtOut->cbFormat < sizeof(VIDEOINFOHEADER))
  {
    return VFW_E_TYPE_NOT_ACCEPTED;
  }

  BITMAPINFOHEADER* lpbiIn = &((VIDEOINFOHEADER*)mtIn->Format())->bmiHeader;
  BITMAPINFOHEADER* lpbiOut = &((VIDEOINFOHEADER*)mtOut->Format())->bmiHeader;

  if (ICERR_OK == ICDecompressQuery(hic_, lpbiIn, lpbiOut)) {
    return S_OK;
  }

  return VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT VfwDecompressor::DecideBufferSize(IMemAllocator* pAlloc, ALLOCATOR_PROPERTIES* ppropInputRequest)
{
  if (!m_pInput->IsConnected())
    return E_UNEXPECTED;

  AM_MEDIA_TYPE mt;

  HRESULT hr = m_pOutput->ConnectionMediaType(&mt);
  if (FAILED(hr))
  {
    return hr;
  }

  ASSERT(mt.formattype == FORMAT_VideoInfo);
  BITMAPINFOHEADER* lpbiOut = HEADER(mt.pbFormat);

  ppropInputRequest->cbBuffer = DIBSIZE(*lpbiOut);
  if (ppropInputRequest->cbAlign == 0)
  {
    ppropInputRequest->cbAlign = 1;
  }
  if (ppropInputRequest->cBuffers == 0)
  {
    ppropInputRequest->cBuffers = 1;
  }
  FreeMediaType(mt);

  ALLOCATOR_PROPERTIES actual;
  hr = pAlloc->SetProperties(ppropInputRequest, &actual);
  if (FAILED(hr))
  {
    return hr;
  }
  if (ppropInputRequest->cbBuffer > actual.cbBuffer)
  {
    hr = E_FAIL;
  }

  return hr;
}

HRESULT VfwDecompressor::GetMediaType(int iPosition, CMediaType* pMediaType)
{
  if (m_pInput->IsConnected() == FALSE)
    return E_UNEXPECTED;

  if (iPosition < 0)
    return E_INVALIDARG;

  if (iPosition > 0)
    return VFW_S_NO_MORE_ITEMS;

  CheckPointer(pMediaType, E_POINTER);
  *pMediaType = m_pInput->CurrentMediaType();

  VIDEOINFOHEADER* vihIn = (VIDEOINFOHEADER*)pMediaType->pbFormat;

  ULONG formatSize = ICDecompressGetFormatSize(hic_, &vihIn->bmiHeader);
  ULONG bufferSize = sizeof(VIDEOINFOHEADER) - sizeof(BITMAPINFOHEADER) + formatSize;

  VIDEOINFOHEADER* vihOut = (VIDEOINFOHEADER*)pMediaType->ReallocFormatBuffer(bufferSize);

  if (ICERR_OK != ICDecompressGetFormat(hic_, &vihIn->bmiHeader, &vihOut->bmiHeader))
    return S_FALSE;

  const GUID guid = GetBitmapSubtype(&vihOut->bmiHeader);

  pMediaType->SetFormatType(&FORMAT_VideoInfo);
  pMediaType->SetSubtype(&guid);
  pMediaType->SetSampleSize(vihOut->bmiHeader.biSizeImage);
  pMediaType->SetTemporalCompression(FALSE);

  return NOERROR;
}
