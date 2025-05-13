#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0602
#endif

#include "DesktopDuplicationCapture.h"

using Microsoft::WRL::ComPtr;

// Constructor ---------------------------------------------------------------
DesktopDuplicationCapture::DesktopDuplicationCapture(UINT monitorIndex, int fovSize)
    : m_valid(false)
    , m_fovSize(fovSize)
    , m_captureWidth(fovSize)
    , m_captureHeight(fovSize)
    , m_offsetX(0)
    , m_offsetY(0)
    , m_d3dDevice(nullptr)
    , m_d3dContext(nullptr)
    , m_duplication(nullptr)
    , m_stagingTexture(nullptr)
    , m_monitorWidth(0)
    , m_monitorHeight(0)
{
    // 1. Getting information about the monitor
    DXGI_OUTPUT_DESC monitorDesc;
    if (!getMonitorInfo(monitorIndex, monitorDesc))
    {
        std::cerr << "[ERROR] Monitor " << monitorIndex << " not found\n";
        return;
    }

    // 2. Calculate offsets for FOV
    RECT monitorRect = monitorDesc.DesktopCoordinates;
    m_monitorWidth = monitorRect.right - monitorRect.left;
    m_monitorHeight = monitorRect.bottom - monitorRect.top;

    m_offsetX = (m_monitorWidth - m_fovSize) / 2;
    m_offsetY = (m_monitorHeight - m_fovSize) / 2;

    if (m_offsetX < 0 || m_offsetY < 0)
    {
        std::cerr << "[ERROR] FOV size exceeds monitor dimensions\n";
        return;
    }

    // 3. Initialization of D3D and Duplication API
    if (!initializeD3D() || !initializeDuplication(monitorIndex))
    {
        std::cerr << "[ERROR] D3D initialization failed\n";
        cleanup();
        return;
    }

    // 4. Preparing staging texture
    if (!createFovStagingTexture())
    {
        std::cerr << "[ERROR] Failed to create staging texture\n";
        cleanup();
        return;
    }

    m_valid = true;
}

// Destructor ----------------------------------------------------------------
DesktopDuplicationCapture::~DesktopDuplicationCapture()
{
    cleanup();
}

// Initializing Direct3D ----------------------------------------------------
bool DesktopDuplicationCapture::initializeD3D()
{
    HRESULT hr = D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0,
        nullptr,
        0,
        D3D11_SDK_VERSION,
        &m_d3dDevice,
        nullptr,
        &m_d3dContext
    );
    return SUCCEEDED(hr);
}

// Getting information about the monitor --------------------------------------------
bool DesktopDuplicationCapture::getMonitorInfo(UINT monitorIndex, DXGI_OUTPUT_DESC& outDesc)
{
    ComPtr<IDXGIFactory1> dxgiFactory;
    if (FAILED(CreateDXGIFactory1(__uuidof(IDXGIFactory1), &dxgiFactory)))
        return false;

    ComPtr<IDXGIAdapter> adapter;
    if (FAILED(dxgiFactory->EnumAdapters(0, &adapter)))
        return false;

    ComPtr<IDXGIOutput> output;
    if (FAILED(adapter->EnumOutputs(monitorIndex, &output)))
        return false;

    return SUCCEEDED(output->GetDesc(&outDesc));
}

// Initializing capture for the specified monitor -----------------------------
bool DesktopDuplicationCapture::initializeDuplication(UINT monitorIndex)
{
    ComPtr<IDXGIDevice> dxgiDevice;
    if (FAILED(m_d3dDevice->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice)))
        return false;

    ComPtr<IDXGIAdapter> dxgiAdapter;
    if (FAILED(dxgiDevice->GetParent(__uuidof(IDXGIAdapter), (void**)&dxgiAdapter)))
        return false;

    ComPtr<IDXGIOutput> dxgiOutput;
    if (FAILED(dxgiAdapter->EnumOutputs(monitorIndex, &dxgiOutput)))
        return false;

    ComPtr<IDXGIOutput1> dxgiOutput1;
    if (FAILED(dxgiOutput->QueryInterface(__uuidof(IDXGIOutput1), (void**)&dxgiOutput1)))
        return false;

    return SUCCEEDED(dxgiOutput1->DuplicateOutput(m_d3dDevice, &m_duplication));
}

// Creating a staging texture for FOV copying -----------------------------
bool DesktopDuplicationCapture::createFovStagingTexture()
{
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = m_fovSize;
    desc.Height = m_fovSize;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_STAGING;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

    return SUCCEEDED(m_d3dDevice->CreateTexture2D(&desc, nullptr, &m_stagingTexture));
}

// Basic frame capture method -----------------------------------------------
cv::Mat DesktopDuplicationCapture::getFovFrame()
{
    cv::Mat frame;
    if (!m_valid) return frame;

    ComPtr<IDXGIResource> desktopResource;
    DXGI_OUTDUPL_FRAME_INFO frameInfo;

    // Frame capture
    HRESULT hr = m_duplication->AcquireNextFrame(100, &frameInfo, &desktopResource);
    if (FAILED(hr)) return frame;

    // Convert to texture
    ComPtr<ID3D11Texture2D> gpuTexture;
    if (FAILED(desktopResource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&gpuTexture)))
    {
        m_duplication->ReleaseFrame();
        return frame;
    }

    // Copy area (center of the screen)
    D3D11_BOX region;
    region.left = m_offsetX;
    region.top = m_offsetY;
    region.right = m_offsetX + m_fovSize;
    region.bottom = m_offsetY + m_fovSize;
    region.front = 0;
    region.back = 1;

    // Copy FOV to staging texture
    m_d3dContext->CopySubresourceRegion(
        m_stagingTexture, 0, 0, 0, 0,
        gpuTexture.Get(), 0, &region
    );

    // Mapping data to the CPU
    D3D11_MAPPED_SUBRESOURCE mapped;
    if (SUCCEEDED(m_d3dContext->Map(m_stagingTexture, 0, D3D11_MAP_READ, 0, &mapped)))
    {
        frame = cv::Mat(m_fovSize, m_fovSize, CV_8UC4, mapped.pData, mapped.RowPitch).clone();
        m_d3dContext->Unmap(m_stagingTexture, 0);
    }

    m_duplication->ReleaseFrame();
    return frame;
}

// Resource cleansing ----------------------------------------------------------
void DesktopDuplicationCapture::cleanup()
{
    if (m_stagingTexture) m_stagingTexture->Release();
    if (m_duplication)    m_duplication->Release();
    if (m_d3dContext)     m_d3dContext->Release();
    if (m_d3dDevice)      m_d3dDevice->Release();

    m_stagingTexture = nullptr;
    m_duplication = nullptr;
    m_d3dContext = nullptr;
    m_d3dDevice = nullptr;
    m_valid = false;
}

// Validity check -------------------------------------------------------
bool DesktopDuplicationCapture::isValid() const
{
    return m_valid;
}