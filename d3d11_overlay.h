#pragma once
#include <windows.h>

struct ID3D11ShaderResourceView;

typedef BOOL(CALLBACK* D3D11_OVERLAY_ON_WNDPROC_CALLBACK)(HWND, UINT, WPARAM, LPARAM);
typedef void(CALLBACK* D3D11_OVERLAY_BEFORE_FRAME_CALLBACK)();
typedef void(CALLBACK* D3D11_OVERLAY_ON_FRAME_CALLBACK)();

namespace D3D11_Overlay
{
	bool Setup(D3D11_OVERLAY_ON_WNDPROC_CALLBACK OnWndProc, D3D11_OVERLAY_BEFORE_FRAME_CALLBACK BeforeFrame, D3D11_OVERLAY_ON_FRAME_CALLBACK OnFrame);
	bool IsRendererReady();
	bool CreateTextureFromFile(const char* path, ID3D11ShaderResourceView** out_srv, int* out_width = nullptr, int* out_height = nullptr);
	void ReleaseTexture(ID3D11ShaderResourceView* texture);
}
