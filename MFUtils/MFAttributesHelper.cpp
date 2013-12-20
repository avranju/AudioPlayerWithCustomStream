#include "pch.h"
#include "MFAsyncCallback.h"

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Storage::Streams;
using namespace Microsoft::WRL;
using namespace concurrency;

// Use these carefully. Only throw in exception based code (C++/CX)
// never throw in HRESULT error code based code.

#define THROW_IF_FAILED(hr)     { if (FAILED(hr)) throw Platform::Exception::CreateException(hr); }
#define RETURN_IF_FAILED(hr)    { if (FAILED(hr)) return hr; }

namespace MFUtils
{

	// This WinRT object provides JavaScript code access to the information in the stream
	// that it needs to construct the AudioEncodingProperties needed to construct the AudioStreamDescriptor
	// needed to create a MediaStreamSource. Here is how to create it
	//      var helper = new MFUtils.MFAttributesHelper();

	public ref class MFAttributesHelper sealed
	{
	private:
		InMemoryRandomAccessStream^ m_stream;
		String^ m_mimeType;

	public:
		MFAttributesHelper(InMemoryRandomAccessStream^ stream, String^ mimeType) :
			m_stream(stream), m_mimeType(m_mimeType)
		{
			THROW_IF_FAILED(MFStartup(MF_VERSION));
		}
		virtual ~MFAttributesHelper()
		{
			MFShutdown();
		}

		property UINT64 Duration;
		property UINT32 BitRate;
		property UINT32 SampleRate;
		property UINT32 ChannelCount;

		void LoadAttributes()
		{
			// create an IMFByteStream from "stream"
			ComPtr<IMFByteStream> byteStream;
			THROW_IF_FAILED(MFCreateMFByteStreamOnStreamEx(reinterpret_cast<IUnknown*>(m_stream), &byteStream));

			// assign mime type to the attributes on this byte stream
			ComPtr<IMFAttributes> attributes;
			THROW_IF_FAILED(byteStream.As(&attributes));
			THROW_IF_FAILED(attributes->SetString(MF_BYTESTREAM_CONTENT_TYPE, m_mimeType->Data()));

			// create a source reader from the byte stream
			ComPtr<IMFSourceReader> sourceReader;
			THROW_IF_FAILED(MFCreateSourceReaderFromByteStream(byteStream.Get(), nullptr, &sourceReader));

			// get current media type
			ComPtr<IMFMediaType> mediaType;
			THROW_IF_FAILED(sourceReader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, &mediaType));

			// get all the data we're looking for
			PROPVARIANT prop;
			THROW_IF_FAILED(sourceReader->GetPresentationAttribute(MF_SOURCE_READER_MEDIASOURCE, MF_PD_DURATION, &prop));
			Duration = prop.uhVal.QuadPart;

			UINT32 data;
			THROW_IF_FAILED(sourceReader->GetPresentationAttribute(MF_SOURCE_READER_MEDIASOURCE, MF_PD_AUDIO_ENCODING_BITRATE, &prop));
			BitRate = prop.ulVal;

			THROW_IF_FAILED(mediaType->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &data));
			SampleRate = data;

			THROW_IF_FAILED(mediaType->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &data));
			ChannelCount = data;
		}
	};
}
