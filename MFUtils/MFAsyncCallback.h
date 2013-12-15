#pragma once
#include <mfidl.h>
#include <mfapi.h>
#include <wrl.h>

template<typename TCallback>
class MFAsyncCallback WrlSealed : public Microsoft::WRL::RuntimeClass<Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>, IMFAsyncCallback, Microsoft::WRL::FtmBase>
{
private:
	TCallback m_callback;

public:
	MFAsyncCallback(TCallback const &callback) : m_callback(callback)
	{
	}

#pragma region IMFAsyncCallback implementation

	STDMETHODIMP GetParameters(DWORD* pdwFlags, DWORD* pdwQueue)
	{
		// Implementation of this method is optional.
		return E_NOTIMPL;
	}

	STDMETHODIMP Invoke(IMFAsyncResult* pAsyncResult)
	{
		return m_callback(pAsyncResult);
	}

#pragma endregion
};

template <typename TCallback>
HRESULT MakeAndInitializeMFAsyncCallback(IMFAsyncCallback **result, TCallback const &callback)
{
	auto obj = Make<MFAsyncCallback<TCallback>>(callback);
	return obj ? obj.CopyTo(result) : E_OUTOFMEMORY;
}
