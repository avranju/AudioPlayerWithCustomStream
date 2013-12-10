#pragma once

#include <ppltasks.h>
#include <mfidl.h>
#include <mfapi.h>
#include <wrl.h>
#include <functional>

class MFAsyncCallback : public RuntimeClass<RuntimeClassFlags<ClassicCom>, IMFAsyncCallback>
{
private:
	std::function<HRESULT(IMFAsyncResult *)> m_callback;
	ComPtr<IMFAttributes> m_attributes;

public:
	MFAsyncCallback(
		std::function<HRESULT(IMFAsyncResult *)> const& callback) :
			m_callback(callback)
	{
	}

	virtual ~MFAsyncCallback() {}

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

