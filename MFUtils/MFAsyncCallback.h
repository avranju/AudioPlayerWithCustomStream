#pragma once

#include <ppltasks.h>
#include <mfidl.h>
#include <mfapi.h>
#include <wrl.h>
#include <functional>

class MFAsyncCallback : public IMFAsyncCallback
{
private:
	std::function<HRESULT(IMFAsyncResult *)> m_callback;
	long m_cRefCount;
	ComPtr<IMFAttributes> m_attributes;

public:
	MFAsyncCallback(
		std::function<HRESULT(IMFAsyncResult *)> const& callback) :
			m_callback(callback),
			m_cRefCount(1)
	{}

	virtual ~MFAsyncCallback() {}

#pragma region IUnknown implementation

	STDMETHODIMP QueryInterface(REFIID riid, void** ppv)
	{
		if (ppv == nullptr)
		{
			return E_POINTER;
		}

		if (riid == __uuidof(IMFAsyncCallback))
		{
			*ppv = static_cast<IMFAsyncCallback*>(this);
			AddRef();
			return S_OK;
		}
		else if (riid == IID_IUnknown)
		{
			*ppv = static_cast<IUnknown*>(this);
			AddRef();
			return S_OK;
		}

		return E_NOINTERFACE;
	}

	STDMETHODIMP_(ULONG) AddRef()
	{
		return InterlockedIncrement(&m_cRefCount);
	}

	STDMETHODIMP_(ULONG) Release()
	{
		long cRef = InterlockedDecrement(&m_cRefCount);
		if (cRef == 0)
		{
			delete this;
		}
		return cRef;
	}

#pragma endregion

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

