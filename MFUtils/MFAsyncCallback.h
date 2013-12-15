#pragma once

#include <ppltasks.h>
#include <mfidl.h>
#include <mfapi.h>
#include <wrl.h>
#include <functional>

template<typename TCallback>
class MFAsyncCallback WrlSealed : public RuntimeClass<RuntimeClassFlags<ClassicCom>, IMFAsyncCallback>
{
private:
	TCallback m_callback;
	ComPtr<IMFAttributes> m_attributes;

public:
	MFAsyncCallback(TCallback const& callback) : m_callback(callback)
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
