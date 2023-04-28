#include <Windows.h>
#include <cstdint>
#include <string>
#include <format>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <cassert>
#include <assert.h>

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")



std::wstring ConvertString(const std::string& str)
{
	if (str.empty())
	{
		return std::wstring();
	}
	auto sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]), static_cast<int>(str.size()), NULL, 0);
	if (sizeNeeded == 0)
	{
		return std::wstring();
	}
	std::wstring result(sizeNeeded, 0);
	MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]), static_cast<int>(str.size()), &result[0], sizeNeeded);
	return result;

}

std::string ConvertString(const std::wstring& str)
{
	if (str.empty())
	{
		return std::string();
	}

	auto sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), NULL, 0, NULL, NULL);
	if (sizeNeeded == 0)
	{
		return std::string();
	}

	std::string result(sizeNeeded, 0);
	WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), result.data(), sizeNeeded, NULL, NULL);
	return result;

}
void Log(const std::string& message)
{
	OutputDebugStringA(message.c_str());
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	//メッセージに応じてゲーム固有の処理を行う
	switch (msg)
	{
	case WM_DESTROY:

		PostQuitMessage(0);


		return 0;


	}

	//標準メッセージ処理を行う
	return DefWindowProc(hwnd, msg, wparam, lparam);

}


int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{

	//OutputDebugStringA("Hello,DirectX!\n");

	WNDCLASS wc{};
	//ウィンドウプロシージャ
	wc.lpfnWndProc = WindowProc;
	//ウィンドウクラス化名（なんでもいい）
	wc.lpszClassName = L"CG2Windowclass";
	//インスタンスハンドル
	wc.hInstance = GetModuleHandle(nullptr);
	//カーソル
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	//ウィンドウクラスを登録する
	RegisterClass(&wc);
	//クライアント領域のサイズ
	const int32_t  kClientWidth = 1280;
	const int32_t  kClientHeight = 720;
	//ウィンドウサイズを表す構造体にクライアント領域をれる
	RECT wrc = { 0,0,kClientWidth,kClientHeight };

	//クライアント領域をもとに実際サイズにwrcを変更してもらう

	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	HWND hwnd = CreateWindow(
		wc.lpszClassName,//利用するクラス名
		L"CG2",//タイトルバーの文字
		WS_OVERLAPPEDWINDOW,//よく見るウィンドウスタイル
		CW_USEDEFAULT,//表示X座標
		CW_USEDEFAULT,//表示Y座標
		wrc.right - wrc.left,//ウィンドウ横幅
		wrc.bottom - wrc.top,//ウィンドウ立幅
		nullptr,//親ウィンドウハンドル
		nullptr,//メニューハンドル
		wc.hInstance,//インスタンスハンドル
		nullptr);//オプション

	//ウィンドウを表示する
	ShowWindow(hwnd, SW_SHOW);

	//DEGIファクトリーの生成
	IDXGIFactory7* dxgiFactory = nullptr;
	//HRESULTはWindowsのエラーコードであり
	//関数が成功したかどうかSUCEEDEDマクロで判定できる
	HRESULT hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory));
	//初期化の根本的な部分的な部分でエラーが出てきた場合間違はプログラムが間違っているかどうにも倍委が多いassertにしておく
	assert(SUCCEEDED(hr));
	//使用するアダプタ用変数。最初にnullpterを入れておく
	IDXGIAdapter4* useAdapter = nullptr;
	//良い順にアダプタ頼む
	for (UINT i = 0; dxgiFactory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&useAdapter)) != DXGI_ERROR_NOT_FOUND; ++i)
	{
		//アダプタ-情報を取得
		DXGI_ADAPTER_DESC3 adapterDesc{};
		hr = useAdapter->GetDesc3(&adapterDesc);
		assert(SUCCEEDED(hr));//取得できないのは一大事
		//ソフトウェアアダプタでなければ採用!
		if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE))
		{
			//採用したアダプタの情報をログに出力　wstringの方なので注意
			Log(ConvertString(std::format(L"Use Adapter{}\n", adapterDesc.Description)));
			break;
		}
		useAdapter = nullptr;//ソフトウェアアダプタの場合は見なかったことにする

	}
	assert(useAdapter != nullptr);//アダプタ起動できない
	ID3D12Device* device = nullptr;
	//機能レベルとログ出力の文字列
	D3D_FEATURE_LEVEL featureLevels[] =
	{

		D3D_FEATURE_LEVEL_12_2, D3D_FEATURE_LEVEL_12_1, D3D_FEATURE_LEVEL_12_0

	};

	const char* featureLevelStrings[] = { "12.2","12.1","12.0" };

	//高い順に生成できるか確かめる
	for (size_t i = 0; i < _countof(featureLevels); i++)
	{
		//採用したアダプターでデバイスを生成
		hr = D3D12CreateDevice(useAdapter, featureLevels[i], IID_PPV_ARGS(&device));
		//指定した機能レベルでデバイスが生成できたか確認
		if (SUCCEEDED(hr))
		{
			Log(std::format("FeatureLevel :{}\n", featureLevelStrings[i]));
			break;
		}
	}

	//デバイスの生成がうまくいかなかったので起動できない
	assert(device != nullptr);
	Log("Complete create D3D12Device!!!\n");//初期化完了のログをだす

	//コマンドキュー設定
	ID3D12CommandQueue* commandQueue = nullptr;
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};

	hr = device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue));
	//コマンドキューの生成がうまく行かなかった場合機能できない
	assert(SUCCEEDED(hr));
	//コマンドアロケータを生成する
	ID3D12CommandAllocator* commandAllocator = nullptr;
	
	hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));
	//コマンドアロケータの生成がうまくいかなかったので機能できない
	assert(SUCCEEDED(hr));

	//コマンドリスト生成
	ID3D12GraphicsCommandList* commandList = nullptr;
	hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator, nullptr, IID_PPV_ARGS(&commandList));
	
	// コマンドリストの生成がうまく行かなかった場合機能できない
	assert(SUCCEEDED(hr));
	
	//スワップチェ－ンを生成
	IDXGISwapChain4* swapChain = nullptr;
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	swapChainDesc.Width = kClientWidth; //画面の幅
	swapChainDesc.Height = kClientWidth;//画面の高さ
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;//色の形式
	swapChainDesc.SampleDesc.Count = 1; //マルチサンプルしない
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; //描画のターゲットとして利用する
	swapChainDesc.BufferCount = 2; //ダブルバッファ
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; //モニタにうつしたら中身破棄
	//コマンドキュー、ウィンドウハンドル、設定渡して生成
	hr = dxgiFactory->CreateSwapChainForHwnd(commandQueue, hwnd, &swapChainDesc, nullptr, nullptr, reinterpret_cast<IDXGISwapChain1**>(&swapChain));
	assert(SUCCEEDED(hr));
	//ディスクリプタヒープの生成
	ID3D12DescriptorHeap* rtvDescriptorHeap = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC rtvDescriptorHeapDesc{};
	rtvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV; //レンダービューゲットビュー用
	rtvDescriptorHeapDesc.NumDescriptors = 2; //ダブルバッファ用に二つ
	hr = device->CreateDescriptorHeap(&rtvDescriptorHeapDesc, IID_PPV_ARGS(&rtvDescriptorHeap));
	assert(SUCCEEDED(hr));

	//swapChainからResourceを引っ張ってくる
	ID3D12Resource* swapChainResource[2] = { nullptr };
	hr = swapChain->GetBuffer(0, IID_PPV_ARGS(&swapChainResource[0]));
	assert(SUCCEEDED(hr));
	hr = swapChain->GetBuffer(1, IID_PPV_ARGS(&swapChainResource[1]));
	assert(SUCCEEDED(hr));


	//STVの設定
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;//出力結果をＳＲＧＢに変換して書きむ
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;//2dテクスチャとして書き込む
	//ディスクリプタの先頭を取得する
	D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandle = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	
	//pTVを二つ作るのでディスプレイを二つ用意
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[2];
	
	//一つ目を最初のところに作る。作る場所をしていする
	rtvHandles[0] = rtvStartHandle;
	device->CreateRenderTargetView(swapChainResource[0], &rtvDesc, rtvHandles[0]);
	
	//二つ目のディスクリプタを作る
	rtvHandles[1].ptr = rtvHandles[0].ptr * device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	//二つ目を作る
	device->CreateRenderTargetView(swapChainResource[1], &rtvDesc, rtvHandles[1]);
	
	//TransitionBarrierの必ず
	//d3d12_RESOURCE_BARRIER barrier{};
	//今回のバリアはTransition
	MSG msg{ };
	//ウィンドウの×ボタンが押されるまでのループ
	while (msg.message != WM_QUIT)
	{
		//Windowにメッセージが来てたら最優先で処理される
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else {
			//これから書き込むバックバッファのインデックスを取得
			UINT backBufferIndex = swapChain->GetCurrentBackBufferIndex();
			//描画先のPTVを設定する
			commandList->OMSetRenderTargets(1, &rtvHandles[backBufferIndex], false, nullptr);
			//指定した色で画面全体をクリアする
			float clearColor[] = { 0.1f,0.25f,0.5f,1.0f };//青っぽい色
			commandList->ClearRenderTargetView(rtvHandles[backBufferIndex], clearColor, 0, nullptr);
			//コマンドの内容を確定させる、すべてのコマンドを積んでからCloseすること
			hr = commandList->Close();
			assert(SUCCEEDED(hr));
			//GPUにコマンドリストの実行を合わせる
			ID3D12CommandList* commandLists[] = { commandList };
			commandQueue->ExecuteCommandLists(1, commandLists);
			//GPUとOSに画面の交換を行うように通知する
			swapChain->Present(1, 0);
			//次のフレーム用コマンドリストを準備
			hr = commandAllocator->Reset();
			assert(SUCCEEDED(hr));
			hr = commandList->Reset(commandAllocator, nullptr);
			assert(SUCCEEDED(hr));
		}
	}




}



