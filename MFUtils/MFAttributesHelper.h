#pragma once

using namespace Windows::Foundation;
using namespace Windows::Storage::Streams;
using namespace concurrency;
using namespace Microsoft::WRL;

namespace MFUtils
{
    public ref class MFAttributesHelper sealed
    {
    public:
		MFAttributesHelper();
		virtual ~MFAttributesHelper();

		property UINT64 Duration;
		property UINT32 BitRate;
		property UINT32 SampleRate;
		property UINT32 ChannelCount;

		IAsyncAction^ LoadAttributesAsync(IRandomAccessStream^ stream, String^ mimeType);

	private:
		task<ComPtr<IMFMediaSource>> CreateMediaSource(
			ComPtr<IMFSourceResolver> const& pResolver,
			ComPtr<IMFByteStream>  const& pStream);
    };
}