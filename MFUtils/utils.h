#pragma once

#include <wrl.h>

// Utility macros and functions from http://kennykerr.ca/2013/01/26/the-api-behind-the-api/

using namespace Microsoft::WRL;
using namespace Platform;

#ifdef DEBUG
#define HR(expression) ASSERT(S_OK == (expression))
#else
inline void HR(HRESULT hr) { if (S_OK != hr) throw Exception::CreateException(hr); }
#endif

template <typename To>
ComPtr<To> winrt_cast(Object ^ from)
{
	ComPtr<To> to;
	HR(reinterpret_cast<IUnknown *>(from)->QueryInterface(to.GetAddressOf()));
	return to;
}
