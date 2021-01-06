#ifndef VFWDECOMPRESSOR_H
#define VFWDECOMPRESSOR_H

#include <vfw.h>

// {7716E5E5-7464-4731-AF75-CE8786C86AB3}
DEFINE_GUID(CLSID_VfwDecompressor,
  0x7716e5e5, 0x7464, 0x4731, 0xaf, 0x75, 0xce, 0x87, 0x86, 0xc8, 0x6a, 0xb3);

class VfwDecompressor : public CTransformFilter
{
public:
  static CUnknown* WINAPI CreateInstance(LPUNKNOWN punk, HRESULT* phr);

  VfwDecompressor();
  ~VfwDecompressor();

  HRESULT	EndOfStream();
  HRESULT Transform(IMediaSample* pIn, IMediaSample* pOut);
  HRESULT CheckInputType(const CMediaType* mtIn);
  HRESULT CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut);
  HRESULT DecideBufferSize(IMemAllocator* pAlloc, ALLOCATOR_PROPERTIES* ppropInputRequest);
  HRESULT GetMediaType(int iPosition, CMediaType* pMediaType);

private:
  HIC		hic_;
  bool	beganDecompress_;
};

#endif