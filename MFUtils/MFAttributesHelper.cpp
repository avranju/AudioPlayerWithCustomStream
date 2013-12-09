// Class1.cpp
#include "pch.h"
#include "utils.h"
#include "MFAttributesHelper.h"
#include "MFAsyncCallback.h"

using namespace MFUtils;
using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Storage::Streams;
using namespace Microsoft::WRL;

MFAttributesHelper::MFAttributesHelper()
{
}

IAsyncAction^ MFAttributesHelper::LoadAttributesAsync(
	IRandomAccessStream^ stream, String^ mimeType)
{
	return create_async([this, stream, mimeType]() {
		// create an IMFByteStream from "stream"
		ComPtr<IMFByteStream> pByteStream;
		auto pUnk = winrt_cast<IUnknown>(stream);
		auto size = stream->Size;
		HR(MFCreateMFByteStreamOnStreamEx(pUnk.Get(), &pByteStream));

		// assign mime type to the attributes on this byte stream
		ComPtr<IMFAttributes> pAttributes;
		HR(pByteStream.As(&pAttributes));
		HR(pAttributes->SetString(MF_BYTESTREAM_CONTENT_TYPE, mimeType->Data()));

		// create an IMFSourceResolver
		ComPtr<IMFSourceResolver> pResolver;
		HR(MFCreateSourceResolver(&pResolver));

		// create an IMFMediaSource asynchronously
		return CreateMediaSource(pResolver, pByteStream).then([this](ComPtr<IMFMediaSource> pSource) {
			// get a presentation descriptor
			ComPtr<IMFPresentationDescriptor> pDescriptor;
			HR(pSource->CreatePresentationDescriptor(&pDescriptor));

			// if descriptor count is not greater than zero we bail
			DWORD descriptorCount;
			HR(pDescriptor->GetStreamDescriptorCount(&descriptorCount));
			if (descriptorCount == 0) {
				throw Exception::CreateException(E_UNEXPECTED);
			}

			// get first stream descriptor
			BOOL selected;
			ComPtr<IMFStreamDescriptor> pStreamDesc;
			HR(pDescriptor->GetStreamDescriptorByIndex(0, &selected, &pStreamDesc));

			// get a media type handler from the stream descriptor
			ComPtr<IMFMediaTypeHandler> pMediaTypeHandler;
			HR(pStreamDesc->GetMediaTypeHandler(&pMediaTypeHandler));

			// if media type count is not greater than zero we bail
			DWORD mediaTypeCount;
			HR(pMediaTypeHandler->GetMediaTypeCount(&mediaTypeCount));
			if (mediaTypeCount == 0) {
				throw Exception::CreateException(E_UNEXPECTED);
			}

			// get current media type
			ComPtr<IMFMediaType> pMediaType;
			HR(pMediaTypeHandler->GetCurrentMediaType(&pMediaType));

			// get all the data we're looking for
			UINT64 duration;
			HR(pDescriptor->GetUINT64(MF_PD_DURATION, &duration));
			Duration = duration;

			UINT32 data;
			HR(pDescriptor->GetUINT32(MF_PD_AUDIO_ENCODING_BITRATE, &data));
			BitRate = data;

			HR(pMediaType->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &data));
			SampleRate = data;

			HR(pMediaType->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &data));
			ChannelCount = data;
		});
	});
}

task<ComPtr<IMFMediaSource>> MFAttributesHelper::CreateMediaSource(
	ComPtr<IMFSourceResolver> pResolver,
	ComPtr<IMFByteStream> pStream)
{
	return create_task([pResolver, pStream]() {
		// this task completion event is used to synchronize MF callback
		// with PPL tasks
		task_completion_event<ComPtr<IMFMediaSource>> tce;

		// create an async callback instance
		ComPtr<IMFAsyncCallback> pCallback = new MFAsyncCallback(
			[pResolver, tce](IMFAsyncResult* pAsyncResult) -> HRESULT {
				ComPtr<IMFMediaSource> pSource;
				ComPtr<IUnknown> pUnk;

				// invoke EndCreateObjectFromByteStream to get the
				// media source object
				MF_OBJECT_TYPE type;
				HR(pResolver->EndCreateObjectFromByteStream(
					pAsyncResult, &type, &pUnk
				));

				// cast to IMFMediaSource and complete the tce
				HR(pUnk.As(&pSource));
				tce.set(pSource);

				return S_OK;
			}
		);

		// start the object creation process
		HR(pResolver->BeginCreateObjectFromByteStream(
			pStream.Get(), nullptr, MF_RESOLUTION_MEDIASOURCE,
			nullptr, nullptr, pCallback.Get(), nullptr
		));

		// wait for the tce to comlete and yield its result
		task<ComPtr<IMFMediaSource>> event_set(tce);
		return event_set;
	});
}