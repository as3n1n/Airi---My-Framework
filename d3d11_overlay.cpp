#include "pch.h"

#include "d3d11_overlay.h"

#include <dxgi.h>
#include <dxgi1_2.h>
#pragma comment(lib, "dxgi.lib")

#include <d3d11.h>
#pragma comment(lib, "d3d11.lib")

#include <wincodec.h>
#pragma comment(lib, "windowscodecs.lib")

#include <wrl.h>
#include <filesystem>
#include <string>
#include <vector>

#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include "global.h"
#include "logger.h"

#include "hook.h"

namespace D3D11_Overlay
{
	static std::string ResolveDefaultMenuFontPath()
	{
		char winDir[MAX_PATH]{};
		GetWindowsDirectoryA(winDir, MAX_PATH);
		const std::filesystem::path fontsDir = std::filesystem::path(winDir) / "Fonts";
		const std::filesystem::path candidates[] =
		{
			fontsDir / "segoeui.ttf",
			fontsDir / "bahnschrift.ttf",
			fontsDir / "segoeuib.ttf",
		};

		for (const std::filesystem::path& candidate : candidates)
		{
			std::error_code ec;
			if (std::filesystem::exists(candidate, ec))
				return candidate.string();
		}

		return (fontsDir / "segoeui.ttf").string();
	}

	static std::string ResolveDefaultMenuTitleFontPath()
	{
		char winDir[MAX_PATH]{};
		GetWindowsDirectoryA(winDir, MAX_PATH);
		const std::filesystem::path fontsDir = std::filesystem::path(winDir) / "Fonts";
		const std::filesystem::path candidates[] =
		{
			fontsDir / "segoeuib.ttf",
			fontsDir / "seguisb.ttf",
			fontsDir / "bahnschrift.ttf",
		};

		for (const std::filesystem::path& candidate : candidates)
		{
			std::error_code ec;
			if (std::filesystem::exists(candidate, ec))
				return candidate.string();
		}

		return ResolveDefaultMenuFontPath();
	}

	static std::wstring Utf8ToWide(const char* text)
	{
		if (!text || !*text) return {};

		int count = MultiByteToWideChar(CP_UTF8, 0, text, -1, nullptr, 0);
		if (count <= 0)
			count = MultiByteToWideChar(CP_ACP, 0, text, -1, nullptr, 0);
		if (count <= 0)
			return {};

		std::wstring result(static_cast<size_t>(count), L'\0');
		if (!MultiByteToWideChar(CP_UTF8, 0, text, -1, result.data(), count))
		{
			if (!MultiByteToWideChar(CP_ACP, 0, text, -1, result.data(), count))
				return {};
		}
		if (!result.empty() && result.back() == L'\0')
			result.pop_back();
		return result;
	}

	static IWICImagingFactory* GetWicFactory()
	{
		static Microsoft::WRL::ComPtr<IWICImagingFactory> factory;
		if (!factory)
		{
			CoInitializeEx(nullptr, COINIT_MULTITHREADED);
			HRESULT hr = CoCreateInstance(
				CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
				IID_PPV_ARGS(&factory));
			if (FAILED(hr))
				return nullptr;
		}
		return factory.Get();
	}

	static D3D11_OVERLAY_ON_WNDPROC_CALLBACK   CurrentOnWndProc = NULL;
	static D3D11_OVERLAY_BEFORE_FRAME_CALLBACK CurrentBeforeFrame = NULL;
	static D3D11_OVERLAY_ON_FRAME_CALLBACK     CurrentOnFrame = NULL;

	static IDXGISwapChain* CurrentSwapChain = NULL;
	static HWND                    CurrentWindow = NULL;
	static ID3D11Device* CurrentDevice = NULL;
	static ID3D11RenderTargetView* CurrentRenderTargetView = NULL;
	static ID3D11DeviceContext* CurrentImmediateContext = NULL;
	static ImGuiContext* CurrentContext = NULL;
	static BOOL                    ImplWin32Initialized = FALSE;
	static BOOL                    ImplDX11Initialized = FALSE;
	static WNDPROC                 CurrentWndProc = NULL;
	static void CleanupRenderer();

	static LRESULT WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		if (CurrentOnWndProc && !CurrentOnWndProc(hWnd, uMsg, wParam, lParam))
			return 0;

		return CurrentWndProc
			? CallWindowProcW(CurrentWndProc, hWnd, uMsg, wParam, lParam)
			: DefWindowProcW(hWnd, uMsg, wParam, lParam);
	}

	static BOOL SetupRenderer(IDXGISwapChain* SwapChain)
	{
		if (!SwapChain) return FALSE;

		static bool s_loggedBegin = false;
		if (!s_loggedBegin) {
			Logger::WriteLine("[Overlay] SetupRenderer begin");
			s_loggedBegin = true;
		}

		CurrentSwapChain = SwapChain;

		DXGI_SWAP_CHAIN_DESC SwapChainDesc;
		if (FAILED(CurrentSwapChain->GetDesc(&SwapChainDesc))) {
			Logger::WriteLine("[Overlay] GetDesc failed");
			CleanupRenderer();
			return FALSE;
		}

		CurrentWindow = SwapChainDesc.OutputWindow;

		if (FAILED(CurrentSwapChain->GetDevice(IID_PPV_ARGS(&CurrentDevice)))) {
			Logger::WriteLine("[Overlay] GetDevice failed");
			CleanupRenderer();
			return FALSE;
		}

		Microsoft::WRL::ComPtr<ID3D11Texture2D> Buffer;
		if (FAILED(SwapChain->GetBuffer(0, IID_PPV_ARGS(&Buffer)))) {
			Logger::WriteLine("[Overlay] GetBuffer failed");
			CleanupRenderer();
			return FALSE;
		}

		if (FAILED(CurrentDevice->CreateRenderTargetView(Buffer.Get(), NULL, &CurrentRenderTargetView))) {
			Logger::WriteLine("[Overlay] CreateRenderTargetView failed");
			CleanupRenderer();
			return FALSE;
		}

		CurrentContext = ImGui::CreateContext();
		ImGui::GetIO().IniFilename = NULL;
		{
			ImGuiIO& io = ImGui::GetIO();
			const std::filesystem::path assets = std::filesystem::current_path() / "assets" / "fonts";
			const std::string bodyPath = ResolveDefaultMenuFontPath();
			const std::string titlePath = ResolveDefaultMenuTitleFontPath();
			const std::string fallbackBodyPath = (assets / "Montserrat-SemiBold.ttf").string();
			const std::string fallbackTitlePath = (assets / "Kodchasan-Bold.ttf").string();

			ImFont* bodyFont = io.Fonts->AddFontFromFileTTF(bodyPath.c_str(), 16.0f);
			if (!bodyFont)
				bodyFont = io.Fonts->AddFontFromFileTTF(fallbackBodyPath.c_str(), 16.0f);
			ImFont* titleFont = io.Fonts->AddFontFromFileTTF(titlePath.c_str(), 29.0f);
			if (!titleFont)
				titleFont = io.Fonts->AddFontFromFileTTF(fallbackTitlePath.c_str(), 29.0f);
			char winDir[MAX_PATH]{};
			GetWindowsDirectoryA(winDir, MAX_PATH);
			const std::filesystem::path iconsFont = std::filesystem::path(winDir) / "Fonts" / "segmdl2.ttf";
			const ImWchar iconRanges[] = { 0xE700, 0xEAFF, 0 };
			io.Fonts->AddFontFromFileTTF(iconsFont.string().c_str(), 18.0f, nullptr, iconRanges);
			if (!bodyFont)
				bodyFont = io.Fonts->AddFontDefault();
			if (bodyFont)
				io.FontDefault = bodyFont;
			if (!bodyFont && !titleFont)
				io.Fonts->AddFontDefault();
		}

		if (!ImGui_ImplWin32_Init(CurrentWindow)) {
			Logger::WriteLine("[Overlay] ImGui_ImplWin32_Init failed");
			CleanupRenderer();
			return FALSE;
		}
		ImplWin32Initialized = TRUE;

		CurrentDevice->GetImmediateContext(&CurrentImmediateContext);
		if (!CurrentImmediateContext) {
			Logger::WriteLine("[Overlay] GetImmediateContext failed");
			CleanupRenderer();
			return FALSE;
		}

		if (!ImGui_ImplDX11_Init(CurrentDevice, CurrentImmediateContext)) {
			Logger::WriteLine("[Overlay] ImGui_ImplDX11_Init failed");
			CleanupRenderer();
			return FALSE;
		}
		ImplDX11Initialized = TRUE;

		CurrentWndProc = (WNDPROC)SetWindowLongPtrW(CurrentWindow, GWLP_WNDPROC, (LONG_PTR)WndProc);
		if (!CurrentWndProc) {
			Logger::WriteLine("[Overlay] SetWindowLongPtrW failed");
			CleanupRenderer();
			return FALSE;
		}

		Logger::WriteLine("[Overlay] Renderer ready (hwnd=%p)", CurrentWindow);

		return TRUE;
	}

	static void CleanupRenderer()
	{
		static bool s_loggedCleanup = false;
		if (!s_loggedCleanup) {
			Logger::WriteLine("[Overlay] CleanupRenderer");
			s_loggedCleanup = true;
		}

		if (CurrentWndProc)
		{
			SetWindowLongPtrW(CurrentWindow, GWLP_WNDPROC, (ULONG_PTR)CurrentWndProc);
			CurrentWndProc = NULL;
		}

		if (ImplDX11Initialized) { ImGui_ImplDX11_Shutdown();  ImplDX11Initialized = FALSE; }
		if (ImplWin32Initialized) { ImGui_ImplWin32_Shutdown(); ImplWin32Initialized = FALSE; }
		if (CurrentContext) { ImGui::DestroyContext(CurrentContext); CurrentContext = NULL; }
		if (CurrentImmediateContext) { CurrentImmediateContext->Release(); CurrentImmediateContext = NULL; }
		if (CurrentRenderTargetView) { CurrentRenderTargetView->Release(); CurrentRenderTargetView = NULL; }
		if (CurrentDevice) { CurrentDevice->Release(); CurrentDevice = NULL; }

		CurrentWindow = NULL;
		CurrentSwapChain = NULL;
	}

	bool IsRendererReady()
	{
		return CurrentContext != NULL && ImplWin32Initialized && ImplDX11Initialized && CurrentRenderTargetView != NULL;
	}

	bool CreateTextureFromFile(const char* path, ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height)
	{
		if (out_srv) *out_srv = nullptr;
		if (out_width) *out_width = 0;
		if (out_height) *out_height = 0;

		if (!CurrentDevice || !path || !*path || !out_srv)
			return false;

		IWICImagingFactory* factory = GetWicFactory();
		if (!factory)
			return false;

		std::wstring widePath = Utf8ToWide(path);
		if (widePath.empty())
			return false;

		Microsoft::WRL::ComPtr<IWICBitmapDecoder> decoder;
		if (FAILED(factory->CreateDecoderFromFilename(
			widePath.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &decoder)))
			return false;

		Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> frame;
		if (FAILED(decoder->GetFrame(0, &frame)))
			return false;

		UINT width = 0;
		UINT height = 0;
		if (FAILED(frame->GetSize(&width, &height)) || width == 0 || height == 0)
			return false;

		Microsoft::WRL::ComPtr<IWICFormatConverter> converter;
		if (FAILED(factory->CreateFormatConverter(&converter)))
			return false;

		if (FAILED(converter->Initialize(
			frame.Get(),
			GUID_WICPixelFormat32bppRGBA,
			WICBitmapDitherTypeNone,
			nullptr,
			0.0f,
			WICBitmapPaletteTypeCustom)))
			return false;

		std::vector<unsigned char> pixels(static_cast<size_t>(width) * static_cast<size_t>(height) * 4ull);
		if (FAILED(converter->CopyPixels(nullptr, width * 4, static_cast<UINT>(pixels.size()), pixels.data())))
			return false;

		D3D11_TEXTURE2D_DESC desc{};
		desc.Width = width;
		desc.Height = height;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.SampleDesc.Count = 1;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

		D3D11_SUBRESOURCE_DATA subresource{};
		subresource.pSysMem = pixels.data();
		subresource.SysMemPitch = width * 4;

		Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
		if (FAILED(CurrentDevice->CreateTexture2D(&desc, &subresource, &texture)))
			return false;

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Format = desc.Format;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;

		if (FAILED(CurrentDevice->CreateShaderResourceView(texture.Get(), &srvDesc, out_srv)))
			return false;

		if (out_width) *out_width = static_cast<int>(width);
		if (out_height) *out_height = static_cast<int>(height);
		return true;
	}

	void ReleaseTexture(ID3D11ShaderResourceView* texture)
	{
		if (texture)
			texture->Release();
	}

	static HOOK DXGISwapChain_PresentHook = {};

	static HRESULT DXGISwapChain_PresentDetour(IDXGISwapChain* _this, UINT SyncInterval, UINT Flags)
	{
		static bool s_loggedFirstPresent = false;
		if (_this && !(Flags & DXGI_PRESENT_TEST))
		{
			if (!CurrentSwapChain)
			{
				if (!SetupRenderer(_this))
					CleanupRenderer();
			}

			if (CurrentSwapChain == _this && CurrentImmediateContext && CurrentRenderTargetView && ImGui::GetCurrentContext())
			{
				if (!s_loggedFirstPresent) {
					Logger::WriteLine("[Overlay] First Present reached");
					s_loggedFirstPresent = true;
				}

				__try
				{
					if (CurrentBeforeFrame) CurrentBeforeFrame();

					ImGui_ImplDX11_NewFrame();
					ImGui_ImplWin32_NewFrame();
					ImGui::NewFrame();

					if (CurrentOnFrame) CurrentOnFrame();

					ImGui::Render();

					CurrentImmediateContext->OMSetRenderTargets(1, &CurrentRenderTargetView, NULL);
					ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
				}
				__except (EXCEPTION_EXECUTE_HANDLER)
				{
					CleanupRenderer();
				}
			}
		}

		return CALL_ORIGINAL(DXGISwapChain_PresentHook, DXGISwapChain_PresentDetour, _this, SyncInterval, Flags);
	}

	static HOOK DXGISwapChain_ResizeBuffersHook = {};

	static HRESULT DXGISwapChain_ResizeBuffersDetour(IDXGISwapChain* _this, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags)
	{
		if (CurrentSwapChain && CurrentSwapChain == _this)
			CleanupRenderer();

		return CALL_ORIGINAL(DXGISwapChain_ResizeBuffersHook, DXGISwapChain_ResizeBuffersDetour, _this, BufferCount, Width, Height, NewFormat, SwapChainFlags);
	}

	static void DXGISwapChain_Setup(IDXGISwapChain* SwapChain)
	{
		PVOID* VTable = *(PVOID**)SwapChain;

		if (!IsHookActive(&DXGISwapChain_PresentHook))
			CreateHook(&DXGISwapChain_PresentHook, VTable[8], DXGISwapChain_PresentDetour, TRUE);

		if (!IsHookActive(&DXGISwapChain_ResizeBuffersHook))
			CreateHook(&DXGISwapChain_ResizeBuffersHook, VTable[13], DXGISwapChain_ResizeBuffersDetour, TRUE);
	}

	static HOOK DXGIFactory_CreateSwapChainHook = {};

	static HRESULT DXGIFactory_CreateSwapChainDetour(IDXGIFactory* _this, IUnknown* pDevice, DXGI_SWAP_CHAIN_DESC* pDesc, IDXGISwapChain** ppSwapChain)
	{
		HRESULT hr = CALL_ORIGINAL(DXGIFactory_CreateSwapChainHook, DXGIFactory_CreateSwapChainDetour, _this, pDevice, pDesc, ppSwapChain);
		if (SUCCEEDED(hr) && ppSwapChain && *ppSwapChain)
			DXGISwapChain_Setup(*ppSwapChain);
		return hr;
	}

	static HOOK DXGIFactory2_CreateSwapChainForHwndHook = {};

	static HRESULT DXGIFactory2_CreateSwapChainForHwndDetour(IDXGIFactory2* _this, IUnknown* pDevice, HWND hWnd, const DXGI_SWAP_CHAIN_DESC1* pDesc, const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* pFullscreenDesc, IDXGIOutput* pRestrictToOutput, IDXGISwapChain1** ppSwapChain)
	{
		HRESULT hr = CALL_ORIGINAL(DXGIFactory2_CreateSwapChainForHwndHook, DXGIFactory2_CreateSwapChainForHwndDetour, _this, pDevice, hWnd, pDesc, pFullscreenDesc, pRestrictToOutput, ppSwapChain);
		if (SUCCEEDED(hr) && ppSwapChain && *ppSwapChain)
			DXGISwapChain_Setup(*ppSwapChain);
		return hr;
	}

	static HOOK DXGIFactory2_CreateSwapChainForCoreWindowHook = {};

	static HRESULT DXGIFactory2_CreateSwapChainForCoreWindowDetour(IDXGIFactory2* _this, IUnknown* pDevice, IUnknown* pWindow, const DXGI_SWAP_CHAIN_DESC1* pDesc, IDXGIOutput* pRestrictToOutput, IDXGISwapChain1** ppSwapChain)
	{
		HRESULT hr = CALL_ORIGINAL(DXGIFactory2_CreateSwapChainForCoreWindowHook, DXGIFactory2_CreateSwapChainForCoreWindowDetour, _this, pDevice, pWindow, pDesc, pRestrictToOutput, ppSwapChain);
		if (SUCCEEDED(hr) && ppSwapChain && *ppSwapChain)
			DXGISwapChain_Setup(*ppSwapChain);
		return hr;
	}

	static HOOK DXGIFactory2_CreateSwapChainForCompositionHook = {};

	static HRESULT DXGIFactory2_CreateSwapChainForCompositionDetour(IDXGIFactory2* _this, IUnknown* pDevice, const DXGI_SWAP_CHAIN_DESC1* pDesc, IDXGIOutput* pRestrictToOutput, IDXGISwapChain1** ppSwapChain)
	{
		HRESULT hr = CALL_ORIGINAL(DXGIFactory2_CreateSwapChainForCompositionHook, DXGIFactory2_CreateSwapChainForCompositionDetour, _this, pDevice, pDesc, pRestrictToOutput, ppSwapChain);
		if (SUCCEEDED(hr) && ppSwapChain && *ppSwapChain)
			DXGISwapChain_Setup(*ppSwapChain);
		return hr;
	}

	static void DXGIFactory_Setup(IDXGIFactory* Factory)
	{
		PVOID* VTable = *(PVOID**)Factory;

		if (!IsHookActive(&DXGIFactory_CreateSwapChainHook))
			CreateHook(&DXGIFactory_CreateSwapChainHook, VTable[10], DXGIFactory_CreateSwapChainDetour, TRUE);

		Microsoft::WRL::ComPtr<IDXGIFactory2> Factory2;
		if (SUCCEEDED(Factory->QueryInterface(IID_PPV_ARGS(&Factory2))))
		{
			PVOID* VTable2 = *(PVOID**)Factory2.Get();

			if (!IsHookActive(&DXGIFactory2_CreateSwapChainForHwndHook))
				CreateHook(&DXGIFactory2_CreateSwapChainForHwndHook, VTable2[15], DXGIFactory2_CreateSwapChainForHwndDetour, TRUE);

			if (!IsHookActive(&DXGIFactory2_CreateSwapChainForCoreWindowHook))
				CreateHook(&DXGIFactory2_CreateSwapChainForCoreWindowHook, VTable2[16], DXGIFactory2_CreateSwapChainForCoreWindowDetour, TRUE);

			if (!IsHookActive(&DXGIFactory2_CreateSwapChainForCompositionHook))
				CreateHook(&DXGIFactory2_CreateSwapChainForCompositionHook, VTable2[24], DXGIFactory2_CreateSwapChainForCompositionDetour, TRUE);
		}
	}

	static HOOK CreateDXGIFactoryHook = {};
	static HOOK CreateDXGIFactory1Hook = {};
	static HOOK CreateDXGIFactory2Hook = {};

	static HRESULT CreateDXGIFactoryDetour(PVOID riid, IDXGIFactory** ppFactory)
	{
		HRESULT hr = CALL_ORIGINAL(CreateDXGIFactoryHook, CreateDXGIFactoryDetour, riid, ppFactory);
		if (SUCCEEDED(hr) && ppFactory && *ppFactory) DXGIFactory_Setup(*ppFactory);
		return hr;
	}

	static HRESULT CreateDXGIFactory1Detour(PVOID riid, IDXGIFactory1** ppFactory)
	{
		HRESULT hr = CALL_ORIGINAL(CreateDXGIFactory1Hook, CreateDXGIFactory1Detour, riid, ppFactory);
		if (SUCCEEDED(hr) && ppFactory && *ppFactory) DXGIFactory_Setup(*ppFactory);
		return hr;
	}

	static HRESULT CreateDXGIFactory2Detour(UINT Flags, PVOID riid, IDXGIFactory2** ppFactory)
	{
		HRESULT hr = CALL_ORIGINAL(CreateDXGIFactory2Hook, CreateDXGIFactory2Detour, Flags, riid, ppFactory);
		if (SUCCEEDED(hr) && ppFactory && *ppFactory) DXGIFactory_Setup(*ppFactory);
		return hr;
	}

	static HOOK D3D11CreateDeviceAndSwapChainHook = {};

	static HRESULT D3D11CreateDeviceAndSwapChainDetour(IDXGIAdapter* pAdapter, D3D_DRIVER_TYPE DriverType, HMODULE Software, UINT Flags, const D3D_FEATURE_LEVEL* pFeatureLevels, UINT FeatureLevels, UINT SDKVersion, const DXGI_SWAP_CHAIN_DESC* pSwapChainDesc, IDXGISwapChain** ppSwapChain, ID3D11Device** ppDevice, D3D_FEATURE_LEVEL* pFeatureLevel, ID3D11DeviceContext** ppImmediateContext)
	{
		HRESULT hr = CALL_ORIGINAL(D3D11CreateDeviceAndSwapChainHook, D3D11CreateDeviceAndSwapChainDetour, pAdapter, DriverType, Software, Flags, pFeatureLevels, FeatureLevels, SDKVersion, pSwapChainDesc, ppSwapChain, ppDevice, pFeatureLevel, ppImmediateContext);
		if (SUCCEEDED(hr) && ppSwapChain && *ppSwapChain)
			DXGISwapChain_Setup(*ppSwapChain);
		return hr;
	}

	// -------------------------------------------------------
	//  Récupère la SwapChain réelle Unity via énumération DXGI
	// -------------------------------------------------------
	static void HookExistingSwapChain()
	{
		Microsoft::WRL::ComPtr<IDXGIFactory1> factory;
		if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))) return;

		// Hook les factories existantes pour les futures SwapChains
		DXGIFactory_Setup(factory.Get());

		// Énumère les adapters pour trouver le device DX11 actif d'Unity
		Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;
		for (UINT i = 0; factory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND; i++, adapter.Reset())
		{
			// Tente de créer un device DX11 sur cet adapter
			// pour obtenir un vtable valide de SwapChain
			Microsoft::WRL::ComPtr<ID3D11Device> tempDevice;
			Microsoft::WRL::ComPtr<IDXGISwapChain> tempSwapChain;
			D3D_FEATURE_LEVEL level;

			HWND hWnd = GetForegroundWindow();
			if (!hWnd) hWnd = GetDesktopWindow();

			DXGI_SWAP_CHAIN_DESC desc = {};
			desc.BufferCount = 1;
			desc.BufferDesc.Width = 8;
			desc.BufferDesc.Height = 8;
			desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			desc.OutputWindow = hWnd;
			desc.SampleDesc.Count = 1;
			desc.Windowed = TRUE;
			desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

			HRESULT hr = D3D11CreateDeviceAndSwapChain(
				adapter.Get(),
				D3D_DRIVER_TYPE_UNKNOWN, // UNKNOWN car on passe un adapter explicite
				nullptr,
				0,
				nullptr,
				0,
				D3D11_SDK_VERSION,
				&desc,
				tempSwapChain.GetAddressOf(),
				tempDevice.GetAddressOf(),
				&level,
				nullptr
			);

			if (SUCCEEDED(hr) && tempSwapChain)
			{
				// Le vtable de tempSwapChain est identique à celui
				// de la SwapChain Unity — on hook Present dessus
				DXGISwapChain_Setup(tempSwapChain.Get());
				return; // Un seul adapter suffit
			}
		}
	}

	bool Setup(D3D11_OVERLAY_ON_WNDPROC_CALLBACK OnWndProc, D3D11_OVERLAY_BEFORE_FRAME_CALLBACK BeforeFrame, D3D11_OVERLAY_ON_FRAME_CALLBACK OnFrame)
	{
		CurrentOnWndProc = OnWndProc;
		CurrentBeforeFrame = BeforeFrame;
		CurrentOnFrame = OnFrame;

		{
			HMODULE hDxgi = LoadLibraryW(L"dxgi.dll");
			if (!hDxgi) return FALSE;

			if (!IsHookActive(&CreateDXGIFactoryHook))
				if (!CreateHook(&CreateDXGIFactoryHook, GetProcAddress(hDxgi, "CreateDXGIFactory"), CreateDXGIFactoryDetour, TRUE))
					return FALSE;

			if (!IsHookActive(&CreateDXGIFactory1Hook))
				if (!CreateHook(&CreateDXGIFactory1Hook, GetProcAddress(hDxgi, "CreateDXGIFactory1"), CreateDXGIFactory1Detour, TRUE))
					return FALSE;

			if (!IsHookActive(&CreateDXGIFactory2Hook))
				if (!CreateHook(&CreateDXGIFactory2Hook, GetProcAddress(hDxgi, "CreateDXGIFactory2"), CreateDXGIFactory2Detour, TRUE))
					return FALSE;
		}

		{
			HMODULE hD3d11 = LoadLibraryW(L"d3d11.dll");
			if (!hD3d11) return FALSE;

			if (!IsHookActive(&D3D11CreateDeviceAndSwapChainHook))
				if (!CreateHook(&D3D11CreateDeviceAndSwapChainHook, GetProcAddress(hD3d11, "D3D11CreateDeviceAndSwapChain"), D3D11CreateDeviceAndSwapChainDetour, TRUE))
					return FALSE;
		}

		// Récupère et hook la SwapChain Unity déjà existante
		HookExistingSwapChain();

		return TRUE;
	}
}
