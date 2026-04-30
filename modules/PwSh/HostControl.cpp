#include "HostControl.hpp"


MyHostControl::MyHostControl(void)
{
    count = 1;

    m_assemblyManager = new MyAssemblyManager();
    m_memoryManager = new MyMemoryManager();
};

MyHostControl::~MyHostControl(void)
{
    if (m_assemblyManager != NULL)
        m_assemblyManager->Release();
    if (m_memoryManager != NULL)
        m_memoryManager->Release();
};


HRESULT STDMETHODCALLTYPE MyHostControl::QueryInterface(REFIID vTableGuid, void** ppv)
{
    // printf("MyHostControl_QueryInterface\n");

    if (ppv == NULL)
        return E_POINTER;

    if (!IsEqualIID(vTableGuid, IID_IUnknown) && !IsEqualIID(vTableGuid, IID_IHostControl))
    {
        *ppv = 0;
        return E_NOINTERFACE;
    }
    *ppv = this;
    this->AddRef();
    return S_OK;
}


ULONG STDMETHODCALLTYPE MyHostControl::AddRef()
{
    // printf("MyHostControl_AddRef\n");

    return static_cast<ULONG>(InterlockedIncrement(&count));
}


ULONG STDMETHODCALLTYPE MyHostControl::Release()
{
    // printf("MyHostControl_Release\n");

    ULONG refCount = static_cast<ULONG>(InterlockedDecrement(&count));
    if (refCount == 0)
    {
        delete this;
        return 0;
    }
    return refCount;
}


// This is responsible for returning all of our manager implementations
// If you want to disable an interface just comment out the if statement
HRESULT STDMETHODCALLTYPE MyHostControl::GetHostManager(REFIID riid, void** ppObject)
{
    // printf("MyHostControl_GetHostManager\n");

    if (ppObject == NULL)
        return E_POINTER;

    if (IsEqualIID(riid, IID_IHostMemoryManager))
    {
        m_memoryManager->AddRef();
        *ppObject = m_memoryManager;
        return S_OK;
    }

    if (IsEqualIID(riid, IID_IHostAssemblyManager))
    {
        m_assemblyManager->AddRef();
        *ppObject = m_assemblyManager;
        return S_OK;
    }

    *ppObject = NULL;
    return E_NOINTERFACE;
}


// //This has some fun uses left as an exercise for the reader :)
HRESULT MyHostControl::SetAppDomainManager(DWORD dwAppDomainID, IUnknown* pUnkAppDomainManager)
{
    // printf("MyHostControl_SetAppDomainManager\n");

    return E_NOTIMPL;
}
