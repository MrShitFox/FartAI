#pragma once
#include <d3d11.h>
#include <dxgi1_2.h>
#include <opencv2/opencv.hpp>
#include <windows.h>
#include <vector>
#include <wrl/client.h>
#include <iostream>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

class DesktopDuplicationCapture
{
public:
    /**
    * @param monitorIndex Monitor index (0 - primary, 1 - secondary, etc.)
    * @param fovSize Size of the square capture area (in pixels)
    */
    DesktopDuplicationCapture(UINT monitorIndex = 0, int fovSize = 256);
    ~DesktopDuplicationCapture();

    bool isValid() const;
    cv::Mat getFovFrame();  // Returns a BGR(A) frame

private:
    bool initializeD3D();
    bool initializeDuplication(UINT monitorIndex);
    bool createFovStagingTexture();
    void cleanup();
    bool getMonitorInfo(UINT monitorIndex, DXGI_OUTPUT_DESC& outDesc);

private:
    bool m_valid;
    int m_fovSize;                // FOV square size
    int m_captureWidth;           // = fovSize
    int m_captureHeight;          // = fovSize
    int m_offsetX;                // X offset for the center of the screen
    int m_offsetY;                // Offset Y

    ID3D11Device* m_d3dDevice;
    ID3D11DeviceContext* m_d3dContext;
    IDXGIOutputDuplication* m_duplication;
    ID3D11Texture2D* m_stagingTexture;

    UINT m_monitorWidth;          // Monitor resolution
    UINT m_monitorHeight;
};