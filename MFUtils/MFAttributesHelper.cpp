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
	public:
		MFAttributesHelper()
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

		IAsyncAction^ LoadAttributesAsync(IRandomAccessStream^ stream, String^ mimeType)
		{
			return create_async([this, stream, mimeType]()
			{
				// create an IMFByteStream from "stream"
				ComPtr<IMFByteStream> byteStream;
				THROW_IF_FAILED(MFCreateMFByteStreamOnStreamEx(reinterpret_cast<IUnknown*>(stream), &byteStream));

				// assign mime type to the attributes on this byte stream
				ComPtr<IMFAttributes> attributes;
				THROW_IF_FAILED(byteStream.As(&attributes));
				THROW_IF_FAILED(attributes->SetString(MF_BYTESTREAM_CONTENT_TYPE, mimeType->Data()));

				// create an IMFMediaSource asynchronously
				return create_task([byteStream]()
				{
					// create an IMFSourceResolver
					ComPtr<IMFSourceResolver> sourceResolver;
					THROW_IF_FAILED(MFCreateSourceResolver(&sourceResolver));

					// this task completion event is used to synchronize MF callback
					// with PPL tasks
					task_completion_event<ComPtr<IMFMediaSource>> completionEvent;

					ComPtr<IMFAsyncCallback> callback;
					THROW_IF_FAILED(MakeAndInitializeMFAsyncCallback(&callback, [sourceResolver, completionEvent](IMFAsyncResult* pAsyncResult) -> HRESULT
					{
						// Get the media source object.
						MF_OBJECT_TYPE type;
						ComPtr<IUnknown> mediaSourceUnk;
						RETURN_IF_FAILED(sourceResolver->EndCreateObjectFromByteStream(pAsyncResult, &type, &mediaSourceUnk));

						// Get the source in ComPtr<IMFMediaSource> form and complete the event.
						ComPtr<IMFMediaSource> mediSource;
						RETURN_IF_FAILED(mediaSourceUnk.As(&mediSource));
						completionEvent.set(mediSource);

						return S_OK;
					}));

					// start the object creation process
					THROW_IF_FAILED(sourceResolver->BeginCreateObjectFromByteStream(
						byteStream.Get(), nullptr, MF_RESOLUTION_MEDIASOURCE,
						nullptr, nullptr, callback.Get(), nullptr));

					// wait for the completionEvent to complete and yield its result
					task<ComPtr<IMFMediaSource>> eventTask(completionEvent);
					return eventTask;
				}).then([this](ComPtr<IMFMediaSource> mediaSource)
				{
					// get a presentation descriptor
					ComPtr<IMFPresentationDescriptor> presentationDesc;
					THROW_IF_FAILED(mediaSource->CreatePresentationDescriptor(&presentationDesc));

					// if descriptor count is not greater than zero we bail
					DWORD descriptorCount;
					THROW_IF_FAILED(presentationDesc->GetStreamDescriptorCount(&descriptorCount));
					if (descriptorCount == 0)
					{
						throw Exception::CreateException(E_UNEXPECTED);
					}

					// get first stream descriptor
					BOOL selected;
					ComPtr<IMFStreamDescriptor> streamDesc;
					THROW_IF_FAILED(presentationDesc->GetStreamDescriptorByIndex(0, &selected, &streamDesc));

					// get a media type handler from the stream descriptor
					ComPtr<IMFMediaTypeHandler> pMediaTypeHandler;
					THROW_IF_FAILED(streamDesc->GetMediaTypeHandler(&pMediaTypeHandler));

					// if media type count is not greater than zero we bail
					DWORD mediaTypeCount;
					THROW_IF_FAILED(pMediaTypeHandler->GetMediaTypeCount(&mediaTypeCount));
					if (mediaTypeCount == 0)
					{
						throw Exception::CreateException(E_UNEXPECTED);
					}

					// get current media type
					ComPtr<IMFMediaType> mediaType;
					THROW_IF_FAILED(pMediaTypeHandler->GetCurrentMediaType(&mediaType));

					// get all the data we're looking for
					UINT64 duration;
					THROW_IF_FAILED(presentationDesc->GetUINT64(MF_PD_DURATION, &duration));
					Duration = duration;

					UINT32 data;
					THROW_IF_FAILED(presentationDesc->GetUINT32(MF_PD_AUDIO_ENCODING_BITRATE, &data));
					BitRate = data;

					THROW_IF_FAILED(mediaType->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &data));
					SampleRate = data;

					THROW_IF_FAILED(mediaType->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &data));
					ChannelCount = data;
				});
			});
		}
	};
}
