#include<d3d12.h>
#include<dxgi1_6.h>
#include<cassert>
#include<d3dcompiler.h>
#include<DirectXMath.h>
#include<math.h>
using namespace DirectX;
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#include<Windows.h>
#include<vector>
#include<string>
#define DIRECTINPUT_VERSION 0x0800
#include<dinput.h>
#pragma comment(lib,"dinput8.lib")
#pragma comment(lib,"dxguid.lib")
#include<DirectXTex.h>
const float PI = 3.141592f;

#include "3dObject.h"
#include <random>

#include<wrl.h>
using namespace Microsoft::WRL;

#pragma region ウィンドウプロシージャ
//ウィンドウプロシージャ
LRESULT WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	//メッセージに応じてゲーム固有の処理を行う
	switch (msg) {
		//ウィンドウが破棄された
		case WM_DESTROY:
			//OSに対してアプリの終了を伝える
			PostQuitMessage(0);
			return 0;
	}
	//標準のメッセージ処理を行う
	return DefWindowProc(hwnd, msg, wparam, lparam);
}
#pragma endregion

//定数バッファ用データ構造体(マテリアル)
struct ConstBufferDataMaterial {
	XMFLOAT4 color;//色(RGBA)
};

//定数バッファ用データ構造体（3D変換行列)
struct ConstBufferDataTransform {
	XMMATRIX mat; //3D変換行列
};

struct Object3d
{
	//定数バッファ（行列用）
	//ID3D12Resource* constBuffTransform;
	ComPtr<ID3D12Resource>constBuffTransform;
	//定数バッファマップ（行列用）
	ConstBufferDataTransform* constMapTransform;
	//アフィン変換情報
	XMFLOAT3 scale = { 1,1,1 };
	XMFLOAT3 rotation = { 0,0,0 };
	XMFLOAT3 position = { 0,0,0 };
	//ワールド変換行列
	XMMATRIX matWorld;
	//親オブジェクトへのポインタ
	Object3d* parent = nullptr;

};

void InitializeObject3d(Object3d* object, ID3D12Device* device);
void UpdateObject3d(Object3d* object, XMMATRIX& matView, XMMATRIX& matProjection);
void DrawObject3d(Object3d* object, ID3D12GraphicsCommandList* commandList, D3D12_VERTEX_BUFFER_VIEW& vbView,
	D3D12_INDEX_BUFFER_VIEW& ibView, UINT numIndices);
void MoveObject3d(Object3d* object, BYTE* key);




int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
	OutputDebugStringA("Hello DirectX!!\n");

#pragma region ウィンドウの生成：設定
	//ウィンドウサイズ
	const int window_width = 1280;
	const int window_height = 720;


	//ウィンドウクラスの生成
	WNDCLASSEX w{};
	w.cbSize = sizeof(WNDCLASSEX);
	w.lpfnWndProc = (WNDPROC)WindowProc;    //ウィンドウプロシージャを指定
	w.lpszClassName = L"DirectXGame";       //ウィンドウクラス名
	w.hInstance = GetModuleHandle(nullptr); //ウィンドウハンドル
	w.hCursor = LoadCursor(NULL, IDC_ARROW);//カーソル指定

	//ウィンドウクラスをOSに登録する
	RegisterClassEx(&w);
	//ウィンドウサイズ{X座標　Y座標　横幅　縦幅}
	RECT wrc = { 0,0,window_width,window_height };
	//自動でサイズを補正する
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);
#pragma endregion

#pragma region ウィンドウの生成：生成
	//ウィンドウオブジェクトの生成
	HWND hwnd = CreateWindow(w.lpszClassName,
		L"DirectXGame",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		wrc.right - wrc.left,
		wrc.bottom - wrc.top,
		nullptr,
		nullptr,
		w.hInstance,
		nullptr);
	//ウィンドウを表示状態にする
	ShowWindow(hwnd, SW_SHOW);
#pragma endregion	

	MSG msg{};//メッセージ

#pragma region DirectXの初期化処理

#ifdef _DEBUG
//デバッグレイヤーをオンに
	ID3D12Debug* debugController;
	//ComPtr<ID3D12Debug>debugController;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
		debugController->EnableDebugLayer();
	}
#endif

	HRESULT result;
	//ID3D12Device* device = nullptr;
	//IDXGIFactory7* dxgiFactory = nullptr;
	//SwapChain4* swapChain = nullptr;
	//ID3D12CommandAllocator* commandAllocator = nullptr;
	//ID3D12GraphicsCommandList* commandList = nullptr;
	//ID3D12CommandQueue* CommandQueue = nullptr;
	//ID3D12DescriptorHeap* rtvHeap = nullptr;

	ComPtr<ID3D12Device>device;
	ComPtr<IDXGIFactory7>dxgiFactory;
	ComPtr<IDXGISwapChain4>swapChain;
	ComPtr<ID3D12CommandAllocator>commandAllocator;
	ComPtr<ID3D12GraphicsCommandList>commandList;
	ComPtr<ID3D12CommandQueue>CommandQueue;
	ComPtr<ID3D12DescriptorHeap>rtvHeap;

#pragma region 初期化処理：アダプタの列拳

#pragma region アダプタの列拳
	//DXGIファクトリーの生成
	result = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory));
	assert(SUCCEEDED(result));

	//アダプターの列挙用
	//std::vector<IDXGIAdapter4*>adapters;
	std::vector<ComPtr<IDXGIAdapter4>>adapters;
	//ここに特定の名前を持つアダプターオブジェクトが入る
	//IDXGIAdapter4* tmpAdapter = nullptr;
	ComPtr<IDXGIAdapter4>tmpAdapter;

	//パフォーマンスが高いものから順に、全てのアダプターを列挙する
	for (UINT i = 0;
		dxgiFactory->EnumAdapterByGpuPreference(i,
			DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
			IID_PPV_ARGS(&tmpAdapter)) != DXGI_ERROR_NOT_FOUND;
		i++) {
		//動的配列に追加する
		adapters.push_back(tmpAdapter);
	}
#pragma endregion

#pragma region アダプタを選別
	//妥当なアダプタを選別する
	for (size_t i = 0; i < adapters.size(); i++) {
		DXGI_ADAPTER_DESC3 adapterDesc;
		//アダプターの情報を取得する
		adapters[i]->GetDesc3(&adapterDesc);

		//ソフトウェアデバイスを回避
		if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)) {
			//デバイスを採用してループを抜ける
			tmpAdapter = adapters[i].Get();
			break;
		}
	}
#pragma endregion

#pragma endregion

#pragma region 初期化処理：デバイスの生成
	//対応レベルの配列
	D3D_FEATURE_LEVEL levels[] = {
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};
	D3D_FEATURE_LEVEL featureLevel;

	for (size_t i = 0; i < _countof(levels); i++) {
		//採用したアダプターでデバイスを生成
		result = D3D12CreateDevice(tmpAdapter.Get(), levels[i],
			IID_PPV_ARGS(&device));
		if (result == S_OK) {
			//デバイスを生成できた時点でループを抜ける
			featureLevel = levels[i];
			break;
		}
	}

#pragma endregion

#pragma region 初期化処理：コマンドリスト
	//コマンドアロケータを生成
	result = device->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(&commandAllocator));
	assert(SUCCEEDED(result));

	//コマンドリストを生成
	result = device->CreateCommandList(0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		commandAllocator.Get(), nullptr,
		IID_PPV_ARGS(&commandList));
	assert(SUCCEEDED(result));
#pragma endregion

#pragma region 初期化処理：コマンドキュー
	//コマンドキューの設定
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
	//コマンドキューを生成
	result = device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&CommandQueue));
	assert(SUCCEEDED(result));
#pragma endregion

#pragma region 初期化処理：スワップチェーン
	//スワップチェーンの設定
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	swapChainDesc.Width = 1280;
	swapChainDesc.Height = 720;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; //色情報の書式
	swapChainDesc.SampleDesc.Count = 1; //マルチサンプルしない
	swapChainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER; //バックバッファ用
	swapChainDesc.BufferCount = 2; //バッファの数を2に設定
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; //フリップ後は破棄
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;


	ComPtr<IDXGISwapChain1>swapChain1;
	//スワップチェーンの生成
	result = dxgiFactory->CreateSwapChainForHwnd(
		CommandQueue.Get(),
		hwnd,
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain1);
	assert(SUCCEEDED(result));

	swapChain1.As(&swapChain);
#pragma endregion

#pragma region 初期化処理：レンダーターゲットビュー
#pragma region デスクリプタヒープ
	//デスクリプタヒープの設定
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV; //レンダーターゲットビュー
	rtvHeapDesc.NumDescriptors = swapChainDesc.BufferCount; //裏表の2つ

	//SRVの最大個数
	const size_t kMaxSRVCount = 2056;

	//デスクリプタヒープの設定
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;//シェーダーから見えるように
	srvHeapDesc.NumDescriptors = kMaxSRVCount;
	//設定をもとにSRV用デスクリプターヒープを生成
	ID3D12DescriptorHeap* srvHeap = nullptr;//----------------------------------------------------------------------------------------
	//ComPtr<ID3D12DescriptorHeap>srvHeap;
	result = device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&srvHeap));
	assert(SUCCEEDED(result));

	//SRVヒープの先頭ハンドルを取得
	D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = srvHeap->GetCPUDescriptorHandleForHeapStart();
	
	//デスクリプターヒープの生成
	device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap));
#pragma endregion

#pragma region バックバッファ
	//バックバッファ
	//std::vector<ID3D12Resource*>backBuffers;
	std::vector<ComPtr<ID3D12Resource>>backBuffers;
	backBuffers.resize(swapChainDesc.BufferCount);
#pragma endregion

#pragma region レンダーターゲットビュー
	//スワップチェーンの全てのバッファについて処理する
	for (size_t i = 0; i < backBuffers.size(); i++) {
		//スワップチェーンからバッファを取得
		swapChain->GetBuffer((UINT)i, IID_PPV_ARGS(&backBuffers[i]));
		//デスクリプタヒープのハンドルを取得
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
		//裏か表かでアドレスがずれる
		rtvHandle.ptr += i * device->GetDescriptorHandleIncrementSize(rtvHeapDesc.Type);
		//レンダーターゲットビューの設定
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
		//シェーダーの計算結果をSRGBに変換して書き込む
		rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		//レンダーターゲットビューの生成
		device->CreateRenderTargetView(backBuffers[i].Get(), &rtvDesc, rtvHandle);
	}
#pragma endregion

#pragma endregion

#pragma region 深度バッファ

	//リソース設定
	D3D12_RESOURCE_DESC depthResourceDesc{};
	depthResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthResourceDesc.Width = window_width;
	depthResourceDesc.Height = window_height;
	depthResourceDesc.DepthOrArraySize = 1;
	depthResourceDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthResourceDesc.SampleDesc.Count = 1;
	depthResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	
	//深度値用ヒーププロパティ
	D3D12_HEAP_PROPERTIES depthHeapProp{};
	depthHeapProp.Type = D3D12_HEAP_TYPE_DEFAULT;
	//深度値のクリア設定
	D3D12_CLEAR_VALUE depthClearValue{};
	depthClearValue.DepthStencil.Depth = 1.0f;
	depthClearValue.Format = DXGI_FORMAT_D32_FLOAT;

	//リソース生成
	//ID3D12Resource* depthBuff = nullptr;
	ComPtr<ID3D12Resource>depthBuff;
	result = device->CreateCommittedResource(
		&depthHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&depthResourceDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthClearValue,
		IID_PPV_ARGS(&depthBuff));

	//深度ビュー用デスクリプタヒープ作成
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	//ID3D12DescriptorHeap* dsvHeap = nullptr;
	ComPtr<ID3D12DescriptorHeap>dsvHeap;
	result = device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsvHeap));

	//深度ビュー作成
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	device->CreateDepthStencilView(
		depthBuff.Get(),
		&dsvDesc,
		dsvHeap->GetCPUDescriptorHandleForHeapStart());


#pragma endregion

#pragma region 初期化処理：フェンス
	//フェンスの生成
	//ID3D12Fence* fence = nullptr;
	ComPtr<ID3D12Fence>fence;
	UINT64 fenceVal = 0;
	result = device->CreateFence(fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));

#pragma endregion

#pragma region キーボード入力の初期化
	//DirectInputの初期化
	IDirectInput8* directInput = nullptr;
	result = DirectInput8Create(
		w.hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8,
		(void**)&directInput, nullptr);
	assert(SUCCEEDED(result));
	//キーボードデバイスの生成
	IDirectInputDevice8* keyboard = nullptr;
	result = directInput->CreateDevice(GUID_SysKeyboard, &keyboard, NULL);
	assert(SUCCEEDED(result));
	//入力データ形式のセット
	result = keyboard->SetDataFormat(&c_dfDIKeyboard);//標準形式
	assert(SUCCEEDED(result));
	//排他制御レベルのセット
	result = keyboard->SetCooperativeLevel(hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
	assert(SUCCEEDED(result));
	struct Vertex
	{
		XMFLOAT3 pos;	//xyz座標
		XMFLOAT3 normal;//法線ベクトル
		XMFLOAT2 uv;	//uv座標
	};
#pragma endregion

#pragma endregion
	
#pragma region 描画初期化処理

#pragma region 頂点データ
	Vertex vertices[] = {
		// x      y     z     法線  u     v
		//前
		{{-5.0f, -5.0f, -5.0f}, {}, {0.0f, 1.0f}}, // 左下
		{{-5.0f,  5.0f, -5.0f}, {}, {0.0f, 0.0f}}, // 左上
		{{ 5.0f, -5.0f, -5.0f}, {}, {1.0f, 1.0f}}, // 右下
		{{ 5.0f,  5.0f, -5.0f}, {}, {1.0f, 0.0f}}, // 右上

		//後
		{{-5.0f,  5.0f,  5.0f}, {}, {0.0f, 0.0f}}, // 左上
		{{-5.0f, -5.0f,  5.0f}, {}, {0.0f, 1.0f}}, // 左下
		{{ 5.0f,  5.0f,  5.0f}, {}, {1.0f, 0.0f}}, // 右上
		{{ 5.0f, -5.0f,  5.0f}, {}, {1.0f, 1.0f}}, // 右下

		//左
		{{-5.0f, -5.0f, -5.0f}, {}, {0.0f, 1.0f}}, // 左下
		{{-5.0f, -5.0f,  5.0f}, {}, {0.0f, 0.0f}}, // 左上
		{{-5.0f,  5.0f, -5.0f}, {}, {1.0f, 1.0f}}, // 右下
		{{-5.0f,  5.0f,  5.0f}, {}, {1.0f, 0.0f}}, // 右上

		//右
		{{ 5.0f, -5.0f,  5.0f}, {}, {0.0f, 0.0f}}, // 左上
		{{ 5.0f, -5.0f, -5.0f}, {}, {0.0f, 1.0f}}, // 左下
		{{ 5.0f,  5.0f,  5.0f}, {}, {1.0f, 0.0f}}, // 右上
		{{ 5.0f,  5.0f, -5.0f}, {}, {1.0f, 1.0f}}, // 右下

		//下
		{{-5.0f,  5.0f, -5.0f}, {}, {0.0f, 1.0f}}, // 左下
		{{-5.0f,  5.0f,  5.0f}, {}, {0.0f, 0.0f}}, // 左上
		{{ 5.0f,  5.0f, -5.0f}, {}, {1.0f, 1.0f}}, // 右下
		{{ 5.0f,  5.0f,  5.0f}, {}, {1.0f, 0.0f}}, // 右上

		//上
		{{-5.0f, -5.0f,  5.0f}, {}, {0.0f, 0.0f}}, // 左上
		{{-5.0f, -5.0f, -5.0f}, {}, {0.0f, 1.0f}}, // 左下
		{{ 5.0f, -5.0f,  5.0f}, {}, {1.0f, 0.0f}}, // 右上
		{{ 5.0f, -5.0f, -5.0f}, {}, {1.0f, 1.0f}}, // 右下

	};

	//頂点データ全体のサイズ=頂点データ一つ分のサイズ*頂点データの要素数
	UINT sizeVB = static_cast<UINT>(sizeof(vertices[0]) * _countof(vertices));
#pragma endregion

#pragma region 頂点インデックス
	unsigned short indices[] = {
		
		//前
		 0,  1,  2,//三角形1つ目
		 2,  1,  3,//三角形2つ目
		//後
		 4,  5,  6,//三角形1つ目
		 6,  5,  7,//三角形2つ目
		//左
		 8,  9, 10,
		 10, 9, 11,
		//右
		12, 13, 14,
		14, 13, 15,
		//下
		16, 17, 18,
		18, 17, 19,
		//上
		20, 21, 22,
		22, 21, 23,
	};
#pragma endregion

#pragma region 頂点バッファの確保
	//頂点バッファの設定
	D3D12_HEAP_PROPERTIES heapProp{};//ヒープ設定
	heapProp.Type = D3D12_HEAP_TYPE_UPLOAD;//GPUへの転送用

	//リソース設定
	D3D12_RESOURCE_DESC resDesc{};
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resDesc.Width = sizeVB;//頂点データ全体のサイズ
	resDesc.Height = 1;
	resDesc.DepthOrArraySize = 1;
	resDesc.MipLevels = 1;
	resDesc.SampleDesc.Count = 1;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	//頂点バッファの生成
	//ID3D12Resource* vertBuff = nullptr;
	ComPtr<ID3D12Resource>vertBuff;
	result = device->CreateCommittedResource(
		&heapProp,//ヒープ設定
		D3D12_HEAP_FLAG_NONE,
		&resDesc,//リソース設定
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&vertBuff));
	assert(SUCCEEDED(result));
#pragma endregion

#pragma region 法線の計算

	for (int i = 0; i < 24 / 3; i++)
	{//三角形1つずつ計算する
		//三角形のインデックスを取り出して、一時的な変数を入れる
		unsigned short index0 = indices[i * 3 + 0];
		unsigned short index1 = indices[i * 3 + 1];
		unsigned short index2 = indices[i * 3 + 2];
		//三角形を構成する頂点座標をベクトルに代入
		XMVECTOR p0 = XMLoadFloat3(&vertices[index0].pos);
		XMVECTOR p1 = XMLoadFloat3(&vertices[index1].pos);
		XMVECTOR p2 = XMLoadFloat3(&vertices[index2].pos);
		//p0→p1ベクトル、p0→p2ベクトルを計算(ベクトルの減算)
		XMVECTOR v1 = XMVectorSubtract(p1, p0);
		XMVECTOR v2 = XMVectorSubtract(p2, p0);
		//外積は両方から垂直なベクトル
		XMVECTOR normal = XMVector3Cross(v1, v2);
		//正規化（長さを1にする）
		normal = XMVector3Normalize(normal);
		//求めた法線を頂点データに代入
		XMStoreFloat3(&vertices[index0].normal, normal);
		XMStoreFloat3(&vertices[index1].normal, normal);
		XMStoreFloat3(&vertices[index2].normal, normal);
	}

#pragma endregion

#pragma region 頂点バッファへのデータ転送
	//GPU上のバッファに対応した仮想メモリ(メインメモリ上)を取得
	Vertex* vertMap = nullptr;
	result = vertBuff->Map(0, nullptr, (void**)&vertMap);
	assert(SUCCEEDED(result));

	//全頂点に対して
	for (int i = 0; i < _countof(vertices); i++) {
		vertMap[i] = vertices[i];//座標をコピー
	}

	//繋がりを解除
	vertBuff->Unmap(0, nullptr);
#pragma endregion

#pragma region 頂点バッファビューの作成
	//頂点バッファビューの作成
	D3D12_VERTEX_BUFFER_VIEW vbView{};

	//GPU仮想アドレス
	vbView.BufferLocation = vertBuff->GetGPUVirtualAddress();

	//頂点バッファのサイズ
	vbView.SizeInBytes = sizeVB;

	//頂点一つ分のデータサイズ
	vbView.StrideInBytes = sizeof(vertices[0]);
	ID3DBlob* vsBlob = nullptr;//頂点シェーダオブジェクト
	ID3DBlob* psBlob = nullptr;//ピクセルシェーダーオブジェクト
	ID3DBlob* errorBlob = nullptr;//エラーオブジェクト
#pragma endregion

#pragma region 頂点シェーダファイルの読み込みとコンパイル
	//頂点シェーダの読み込みとコンパイル
	result = D3DCompileFromFile(
		L"BasicVS.hlsl",//シェーダファイル名
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,//インクルード可能にする
		"main", "vs_5_0",//エントリーポイント名、シェーダーモデル指定
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,//デバッグ用設定
		0,
		&vsBlob, &errorBlob);

#pragma region シェーダーコードのエラー
	//エラーなら
	if (FAILED(result)) {
		//errorBlobからエラー内容をstring型にコピー
		std::string error;
		error.resize(errorBlob->GetBufferSize());

		std::copy_n((char*)errorBlob->GetBufferPointer(),
			errorBlob->GetBufferSize(),
			error.begin());
		error += "\n";
		//エラー内容を出力ウィンドウに表示
		OutputDebugStringA(error.c_str());
		assert(0);
	}
#pragma endregion
#pragma endregion

#pragma region ピクセルシェーダの読み込みとコンパイル
	//ピクセルシェーダの読み込みとコンパイル
	result = D3DCompileFromFile(
		L"BasicPS.hlsl",//シェーダファイル名
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,//インクルード可能にする
		"main", "ps_5_0",//エントリーポイント名、シェーダーモデル指定
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,//デバッグ用設定
		0,
		&psBlob, &errorBlob);

#pragma region	シェーダーコードのエラー	
	//エラーなら
	if (FAILED(result)) {
		//errorBlobからエラー内容をstring型にコピー
		std::string error;
		error.resize(errorBlob->GetBufferSize());

		std::copy_n((char*)errorBlob->GetBufferPointer(),
			errorBlob->GetBufferSize(),
			error.begin());
		error += "\n";
		//エラー内容を出力ウィンドウに表示
		OutputDebugStringA(error.c_str());
		assert(0);
	}
#pragma endregion
#pragma endregion

#pragma region 頂点レイアウト
	//頂点レイアウト
	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{///xyz座標
			"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0
		},

		{//法線ベクトル
			"NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0
		},

		{//uv座標
			"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0
		},
	};

#pragma endregion

#pragma region グラフィックスパイプライン
	//グラフィックスパイプライン設定
	D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineDesc{};

	//シェーダーの設定
	pipelineDesc.VS.pShaderBytecode = vsBlob->GetBufferPointer();
	pipelineDesc.VS.BytecodeLength = vsBlob->GetBufferSize();
	pipelineDesc.PS.pShaderBytecode = psBlob->GetBufferPointer();
	pipelineDesc.PS.BytecodeLength = psBlob->GetBufferSize();

	//サンプルマスクの設定
	pipelineDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;//標準設定

	//ラスタライザの設定
	pipelineDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;//カリング
	pipelineDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;//ポリゴン内塗りつぶし
	pipelineDesc.RasterizerState.DepthClipEnable = true;//深度クリッピングを有効に

#pragma region ブレンドステート
	//ブレンドステート
	D3D12_RENDER_TARGET_BLEND_DESC& blenddesc = pipelineDesc.BlendState.RenderTarget[0];
	blenddesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	//共通設定
	blenddesc.BlendEnable = true;//ブレンドを有効にする
	blenddesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;//加算
	blenddesc.SrcBlendAlpha = D3D12_BLEND_ONE;//ソースの値を100％使う
	blenddesc.DestBlendAlpha = D3D12_BLEND_ZERO;//デストの値を0％使う

	//半透明合成
	blenddesc.BlendOp = D3D12_BLEND_OP_ADD;//加算
	blenddesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;//ソースのアルファ値
	blenddesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;//1.0f-ソースのアルファ値
#pragma endregion

	//頂点レイアウトの設定
	pipelineDesc.InputLayout.pInputElementDescs = inputLayout;
	pipelineDesc.InputLayout.NumElements = _countof(inputLayout);

	//図形の形状設定
	pipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	//デプスステンシルステートの設定
	pipelineDesc.DepthStencilState.DepthEnable = true;
	pipelineDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	pipelineDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	pipelineDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

	//デスクリプタレンジの設定
	D3D12_DESCRIPTOR_RANGE descriptorRange{};
	descriptorRange.NumDescriptors = 1;//一度の描画に使うテクスチャが１枚なので１
	descriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRange.BaseShaderRegister = 0; //テクスチャレジスタ0番
	descriptorRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	//その他の設定
	pipelineDesc.NumRenderTargets = 1;//描画対象は一つ
	pipelineDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;//0~255指定のRGBA
	pipelineDesc.SampleDesc.Count = 1;//1ピクセルにつき一回サンプリング

#pragma region ルートパラメータ
	//ルートパラメータの設定
	D3D12_ROOT_PARAMETER rootParams[3] = {};

	//定数バッファ0番
	rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;//定数バッファビュー
	rootParams[0].Descriptor.ShaderRegister = 0;//定数バッファ番号
	rootParams[0].Descriptor.RegisterSpace = 0;//デフォルト値
	rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;//全てのシェーダから見える

	//テクスチャレジスタ0番
	rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParams[1].DescriptorTable.pDescriptorRanges = &descriptorRange;
	rootParams[1].DescriptorTable.NumDescriptorRanges = 1;
	rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	//定数バッファ1番
	rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParams[2].Descriptor.ShaderRegister = 1;
	rootParams[2].Descriptor.RegisterSpace = 0;
	rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
#pragma endregion

#pragma region テクスチャサンプラー
	//テクスチャサンプラーの設定
	D3D12_STATIC_SAMPLER_DESC samplerDesc{};
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;//横繰り返し(タイリング)
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;//縦繰り返し（タイリング）
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;//奥行繰り返し（タイリング）
	samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;//ボーダーの時は黒
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;//全てシニア補間
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;//ミップマップ最大値
	samplerDesc.MinLOD = 0.0f;//ミップマップ最小値
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;//ピクセルシェーダからのみ使用可能
#pragma endregion

#pragma region ルートシグネチャ
	//ルートシグネチャ
	//ID3D12RootSignature* rootSignature;
	ComPtr<ID3D12RootSignature>rootSignature;

	//ルートシグネチャの設定
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rootSignatureDesc.pParameters = rootParams;//ルートパラメータの先頭アドレス
	rootSignatureDesc.NumParameters = _countof(rootParams);//ルートパラメータ数
	rootSignatureDesc.pStaticSamplers = &samplerDesc;
	rootSignatureDesc.NumStaticSamplers = 1;

	//シグネチャのシリアライズ
	//ID3D10Blob* rootSigBlob = nullptr;
	ComPtr<ID3D10Blob>rootSigBlob;
	result = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &rootSigBlob, &errorBlob);
	assert(SUCCEEDED(result));
	result = device->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
	assert(SUCCEEDED(result));

	//パイプラインにルートシグネチャをセット
	pipelineDesc.pRootSignature = rootSignature.Get();
#pragma endregion

	//パイプラインステートの生成
	//ID3D12PipelineState* pipelineState = nullptr;
	ComPtr<ID3D12PipelineState>pipelineState;
	result = device->CreateGraphicsPipelineState(&pipelineDesc, IID_PPV_ARGS(&pipelineState));
	assert(SUCCEEDED(result));

#pragma endregion

#pragma region 定数バッファ
	//定数バッファ用データ構造体(マテリアル)
	struct ConstBufferDataMaterial {
		XMFLOAT4 color;//色(RGBA)
	};


#pragma region Material
	//ヒープ設定
	//ID3D12Resource* constBuffMaterial = nullptr;
	ComPtr<ID3D12Resource>constBuffMaterial;
	ConstBufferDataMaterial* constMapMaterial = nullptr;
	{
		D3D12_HEAP_PROPERTIES cbHeapProp{};
		cbHeapProp.Type = D3D12_HEAP_TYPE_UPLOAD;//GPUへの転送用

		//リソース設定

		D3D12_RESOURCE_DESC cbResourceDesc{};
		cbResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		cbResourceDesc.Width = (sizeof(ConstBufferDataMaterial) + 0xff) & ~0xff;
		cbResourceDesc.Height = 1;
		cbResourceDesc.DepthOrArraySize = 1;
		cbResourceDesc.MipLevels = 1;
		cbResourceDesc.SampleDesc.Count = 1;
		cbResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;


		//定数バッファの生成
		result = device->CreateCommittedResource(
			&cbHeapProp,//ヒープ設定
			D3D12_HEAP_FLAG_NONE,
			&cbResourceDesc,//リソース設定
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&constBuffMaterial));
		assert(SUCCEEDED(result));

		//定数バッファのマッピング

		result = constBuffMaterial->Map(0, nullptr, (void**)&constMapMaterial);//マッピング
		assert(SUCCEEDED(result));
	}
#pragma endregion

#pragma region Transform
	

	//3Dオブジェクトの数
	const size_t kObjectCount = 50;
	//3Dオブジェクトの配列
	Object3d object3ds[kObjectCount];

	std::random_device seed_gen;
	std::mt19937_64 engine(seed_gen());
	std::uniform_real_distribution<float>ranX(-60.0f, 60.0f);
	std::uniform_real_distribution<float>ranY(-60.0f, 60.0f);
	std::uniform_real_distribution<float>ranZ(-60.0f, 60.0f);

	for (int i = 0; i < _countof(object3ds); i++)
	{
		//初期化
		InitializeObject3d(&object3ds[i], device.Get());
		//ここからは親子構造のサンプル
		//先頭以外なら
		if(i > 0){
			//一つ前のオブジェクトを親オブジェクトとする
			//object3ds[i].parent = &object3ds[i - 1];
			//親オブジェクトの9割の大きさ
			object3ds[i].scale = { 0.9f,0.9f,0.9f };
			//親オブジェクトに対してZ軸まわりに３０度回転
			float randomX = ranX(engine);
			float randomY = ranY(engine);
			float randomZ = ranZ(engine);
			object3ds[i].rotation = { XMConvertToRadians(randomX),XMConvertToRadians(randomY),XMConvertToRadians(randomZ) };
			//親オブジェクトに対してZ方向に-8.0ずらす
			object3ds[i].position = { randomX,randomY,randomZ };
		}
	}
#pragma endregion

	//値を書き込むと自動的に転送される
	constMapMaterial->color = XMFLOAT4(1, 1, 1, 1.0f);

#pragma region 射影変換
	//投資投影行列の計算
	XMMATRIX matProjection =
		XMMatrixPerspectiveFovLH(
			XMConvertToRadians(45.0f),
			(float)window_width / window_height,
			0.1f, 1000.0f
		);


#pragma endregion

#pragma region ビュー変換
	//ビュー変換行列
	XMMATRIX matView;

	XMFLOAT3 eye(0, 0, -100);

	XMFLOAT3 target(0, 0, 0);

	XMFLOAT3 up(0, 1, 0);

	float angle = 0.0f;

	matView = XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&up));
#pragma endregion



#pragma endregion

#pragma region 画像イメージデータの作成
	//横方向ピクセル数
	const size_t textureWidth = 256;

	//縦方向ピクセル数
	const size_t textureHeight = 256;

	//配列の要素数
	const size_t imageDataCount = textureWidth * textureHeight;

	//1枚目のテクスチャ
	//画像イメージデータ配列
	TexMetadata metadata{};
	ScratchImage scratchImg{};

	// WICテクスチャのロード
	result = LoadFromWICFile(
		L"Resources/pien.jpg",   //「Resources」フォルダの「texture.png」
		WIC_FLAGS_NONE,
		&metadata, scratchImg);


	//2枚目のテクスチャ
	//画像イメージデータ配列
	TexMetadata metadata2{};
	ScratchImage scratchImg2{};

	// WICテクスチャのロード
	result = LoadFromWICFile(
		L"Resources/reimu.png",   //「Resources」フォルダの「texture.png」
		WIC_FLAGS_NONE,
		&metadata2, scratchImg2);

	
	


	ScratchImage mipChain{};
	// ミップマップ生成
	result = GenerateMipMaps(
		scratchImg.GetImages(), scratchImg.GetImageCount(), scratchImg.GetMetadata(),
		TEX_FILTER_DEFAULT, 0, mipChain);
	if (SUCCEEDED(result)) {
		scratchImg = std::move(mipChain);
		metadata = scratchImg.GetMetadata();
	}

	// ミップマップ生成
	result = GenerateMipMaps(
		scratchImg2.GetImages(), scratchImg2.GetImageCount(), scratchImg2.GetMetadata(),
		TEX_FILTER_DEFAULT, 0, mipChain);
	if (SUCCEEDED(result)) {
		scratchImg2 = std::move(mipChain);
		metadata2 = scratchImg2.GetMetadata();
	}



	// 読み込んだディフューズテクスチャをSRGBとして扱う
	metadata.format = MakeSRGB(metadata.format);

	// 読み込んだディフューズテクスチャをSRGBとして扱う
	metadata2.format = MakeSRGB(metadata2.format);

	XMFLOAT4* imageData = new XMFLOAT4[imageDataCount];//かならず後で開放する
#pragma endregion

#pragma region テクスチャバッファの生成
	//ヒープ設定
	D3D12_HEAP_PROPERTIES textureHeapProp{};
	textureHeapProp.Type = D3D12_HEAP_TYPE_CUSTOM;
	textureHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	textureHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;

	// リソース設定(1枚目)
	D3D12_RESOURCE_DESC textureResourceDesc{};
	textureResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	textureResourceDesc.Format = metadata.format;
	textureResourceDesc.Width = metadata.width;
	textureResourceDesc.Height = (UINT)metadata.height;
	textureResourceDesc.DepthOrArraySize = (UINT16)metadata.arraySize;
	textureResourceDesc.MipLevels = (UINT16)metadata.mipLevels;
	textureResourceDesc.SampleDesc.Count = 1;

	// リソース設定(2枚目)
	D3D12_RESOURCE_DESC textureResourceDesc2{};
	textureResourceDesc2.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	textureResourceDesc2.Format = metadata2.format;
	textureResourceDesc2.Width = metadata2.width;
	textureResourceDesc2.Height = (UINT)metadata2.height;
	textureResourceDesc2.DepthOrArraySize = (UINT16)metadata2.arraySize;
	textureResourceDesc2.MipLevels = (UINT16)metadata2.mipLevels;
	textureResourceDesc2.SampleDesc.Count = 1;



	//1枚目のテクスチャ
	//テクスチャバッファの生成
	//ID3D12Resource* texBuff = nullptr;
	ComPtr<ID3D12Resource>texBuff;
	result = device->CreateCommittedResource(
		&textureHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&textureResourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&texBuff));

	//2枚目のテクスチャ
	//テクスチャバッファの生成
	//ID3D12Resource* texBuff2 = nullptr;
	ComPtr<ID3D12Resource>texBuff2;
	result = device->CreateCommittedResource(
		&textureHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&textureResourceDesc2,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&texBuff2));

	// 全ミップマップについて
	for (size_t i = 0; i < metadata.mipLevels; i++) {
		// ミップマップレベルを指定してイメージを取得
		const Image* img = scratchImg.GetImage(i, 0, 0);
		// テクスチャバッファにデータ転送

	//テクスチャバッファにデータ転送
		result = texBuff->WriteToSubresource(
			(UINT)i,
			nullptr,              // 全領域へコピー
			img->pixels,          // 元データアドレス
			(UINT)img->rowPitch,  // 1ラインサイズ
			(UINT)img->slicePitch // 1枚サイズ
		);
		assert(SUCCEEDED(result));
	}

	// 全ミップマップについて
	for (size_t i = 0; i < metadata2.mipLevels; i++) {
		// ミップマップレベルを指定してイメージを取得
		const Image* img = scratchImg2.GetImage(i, 0, 0);
		// テクスチャバッファにデータ転送

	//テクスチャバッファにデータ転送
		result = texBuff2->WriteToSubresource(
			(UINT)i,
			nullptr,              // 全領域へコピー
			img->pixels,          // 元データアドレス
			(UINT)img->rowPitch,  // 1ラインサイズ
			(UINT)img->slicePitch // 1枚サイズ
		);
		assert(SUCCEEDED(result));
	}


	//元データ開放
	delete[] imageData;
#pragma endregion

#pragma region シェーダリソースビューの作成
	// シェーダリソースビュー設定
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = textureResourceDesc.Format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = textureResourceDesc.MipLevels;

	//ハンドルのさす位置にシェーダーリソースビュー作成
	device->CreateShaderResourceView(texBuff.Get(), &srvDesc, srvHandle);

	//1つハンドルを進める
	UINT incrementSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	srvHandle.ptr += incrementSize;
	//srvHandle.ptr = 0;

	// シェーダリソースビュー設定
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc2{};
	srvDesc2.Format = textureResourceDesc2.Format;
	srvDesc2.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc2.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc2.Texture2D.MipLevels = textureResourceDesc2.MipLevels;

	//ハンドルのさす位置にシェーダーリソースビュー作成
	device->CreateShaderResourceView(texBuff2.Get(), &srvDesc2, srvHandle);


#pragma endregion
	
#pragma region インデックスバッファ

#pragma region インデックスバッファの設定
	//インデックスデータ全体のサイズ
	UINT sizeIB = static_cast<UINT>(sizeof(uint16_t) * _countof(indices));

	//リソース設定
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resDesc.Width = sizeIB;//インデックスの情報が入る分のサイズ
	resDesc.Height = 1;
	resDesc.DepthOrArraySize = 1;
	resDesc.MipLevels = 1;
	resDesc.SampleDesc.Count = 1;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	//インデックスバッファの生成
	//ID3D12Resource* indexBuff = nullptr;
	ComPtr<ID3D12Resource>indexBuff;
	result = device->CreateCommittedResource(
		&heapProp,//ヒープ設定
		D3D12_HEAP_FLAG_NONE,
		&resDesc,//リソース設定
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&indexBuff));
#pragma endregion

#pragma region インデックスバッファへのデータ転送
	//インデックスバッファをマッピング
	uint16_t* indexMap = nullptr;
	result = indexBuff->Map(0, nullptr, (void**)&indexMap);

	//全インデックスに対して
	for (int i = 0; i < _countof(indices); i++) {
		indexMap[i] = indices[i];//インデックスをコピー
	}

	//マッピング解除
	indexBuff->Unmap(0, nullptr);
#pragma endregion

#pragma region インデックスバッファビューの生成
	//インデックスバッファビューの生成
	D3D12_INDEX_BUFFER_VIEW ibView{};
	ibView.BufferLocation = indexBuff->GetGPUVirtualAddress();
	ibView.Format = DXGI_FORMAT_R16_UINT;
	ibView.SizeInBytes = sizeIB;
#pragma endregion

#pragma endregion
	

#pragma endregion
	BYTE keys[256] = {};
	BYTE oldkeys[256] = {};
	//ゲームループ
	while (true) {

#pragma region ウィンドウメッセージ処理
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);//キー入力のメッセージ処理
			DispatchMessage(&msg);//プロシージャにメッセージを送る
		}

		//xボタンで終了メッセージが来たらゲームループを抜ける
		if (msg.message == WM_QUIT) {
			break;
		}
#pragma endregion
		
#pragma region DirectX毎フレーム処理
		//DirectX毎フレーム処理　ここから
		//キーボード情報の取得
		keyboard->Acquire();
		for (int i = 0; i < 256; i++)
		{
			oldkeys[i] = keys[i];
		}
		//全キーの入力状態を取得する
		keyboard->GetDeviceState(sizeof(keys), keys);

#pragma region ビュー変換
		if (keys[DIK_D] || keys[DIK_A])
		{
			if (keys[DIK_D])
			{
				angle += XMConvertToRadians(1.0f);
			}
			else if (keys[DIK_A])
			{
				angle -= XMConvertToRadians(1.0f);
			}

			//angleラジアンだけy軸まわりに回転。半径は-100
			eye.x = -100 * sinf(angle);
			eye.z = -100 * cosf(angle);

			matView = XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&up));
		}
#pragma endregion
		
#pragma region ワールド変換

		for (size_t i = 0; i < _countof(object3ds); i++)
		{
			UpdateObject3d(&object3ds[i], matView, matProjection);
		}
		MoveObject3d(&object3ds[0], keys);
			
#pragma endregion
		
#pragma region テクスチャの切り替え
		if (keys[DIK_SPACE] == 0x80 && oldkeys[DIK_SPACE] == 0x00)
		{
			if (incrementSize != 0)
			{
				incrementSize = 0;
			}
			else
			{
				incrementSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			}
		}
		
#pragma endregion

		//バックバッファの番号を取得(2つなので0番か1番)
		UINT bbIndex = swapChain->GetCurrentBackBufferIndex();

#pragma endregion

#pragma region グラフィックスコマンド

#pragma region リソースバリアの変更コマンド
		//1.リソースバリアで書き込み可能に変更
		D3D12_RESOURCE_BARRIER barrierDesc{};
		barrierDesc.Transition.pResource = backBuffers[bbIndex].Get(); //バックバッファを指定
		barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT; //表示状態から 
		barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET; //描画状態へ
		commandList->ResourceBarrier(1, &barrierDesc);
#pragma endregion

#pragma region レンダーターゲットビュー設定コマンド
		//2.描画先の変更
		//レンダーターゲットビューのハンドルを取得
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
		rtvHandle.ptr += bbIndex * device->GetDescriptorHandleIncrementSize(rtvHeapDesc.Type);
		//深度ステンシルビュー用デスクリプタヒープのハンドルを取得
		D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvHeap->GetCPUDescriptorHandleForHeapStart();
		commandList->OMSetRenderTargets(1, &rtvHandle, false, &dsvHandle);

		//commandList->OMSetRenderTargets(1, &rtvHandle, false, nullptr);
#pragma endregion

#pragma region 画面クリアコマンド
		//3.画面クリア
		FLOAT clearColor[] = { 0.1f,0.25f,0.5f,0.0f };//青っぽい色
		commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
		commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
#pragma endregion

#pragma region 描画コマンド
		
#pragma region ビューポート設定
		//ビューポート設定コマンド
		D3D12_VIEWPORT viewport{};
		viewport.Width = window_width;
		viewport.Height = window_height;
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth - 1.0f;

		//ビューポート設定コマンドを、コマンドリストに積む
		commandList->RSSetViewports(1, &viewport);
#pragma endregion

#pragma region シザー矩形設定
		//シザー矩形
		D3D12_RECT scissorRect{};
		scissorRect.left = 0;//切り抜き座標左
		scissorRect.right = scissorRect.left + window_width;//切り抜き座標右
		scissorRect.top = 0;//切り抜き座標上
		scissorRect.bottom = scissorRect.top + window_height;//切り抜き座標下

		//シザー矩形設定コマンドを、コマンドリストに積む
		commandList->RSSetScissorRects(1, &scissorRect);
#pragma endregion

#pragma region パイプラインステート設定
		//パイプラインステートとルートシグネチャの設定コマンド
		commandList->SetPipelineState(pipelineState.Get());
		commandList->SetGraphicsRootSignature(rootSignature.Get());
#pragma endregion

#pragma region プリミティブ型状
		//プリミティブ型状の設定コマンド
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);//三角形リスト
#pragma endregion

#pragma region 頂点バッファビューの設定
		//頂点バッファビューの設定コマンド
		commandList->IASetVertexBuffers(0, 1, &vbView);
#pragma endregion

#pragma region インデックスバッファビューのセットコマンド
		//インデックスバッファビューの設定コマンド
		commandList->IASetIndexBuffer(&ibView);
#pragma endregion

#pragma region CBVの設定コマンド
		//定数バッファビュー(CBV)の設定コマンド
		commandList->SetGraphicsRootConstantBufferView(0, constBuffMaterial->GetGPUVirtualAddress());
#pragma endregion

#pragma region SRVヒープ
		//SRVヒープの設定コマンド
		commandList->SetDescriptorHeaps(1, &srvHeap);
		//SRVヒープの先頭ハンドルを取得(SRVを指しているはず)
		D3D12_GPU_DESCRIPTOR_HANDLE srvGpuHandle = srvHeap->GetGPUDescriptorHandleForHeapStart();//-----------------------------------------------
		
		//2枚目を指し示すようにしたSRVハンドルをルートパラメータ1番に設定
		srvGpuHandle.ptr += incrementSize;
		
		commandList->SetGraphicsRootDescriptorTable(1, srvGpuHandle);


		//全オブジェクトについて処理
		for (int i = 0; i < _countof(object3ds); i++)
		{
			DrawObject3d(&object3ds[i], commandList.Get(), vbView, ibView, _countof(indices));
		}
		
#pragma endregion
	
#pragma endregion

#pragma region リソースバリアの復帰コマンド
		//5.リソースバリアを戻す
		barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET; //描画状態から
		barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		commandList->ResourceBarrier(1, &barrierDesc);
#pragma endregion

#pragma region コマンドのフラッシュ
		//命令のクローズ
		result = commandList->Close();
		assert(SUCCEEDED(result));

		//コマンドリストの実行
		ID3D12CommandList* commandLists[] = { commandList.Get()};
		CommandQueue->ExecuteCommandLists(1, commandLists);

		//画面に表示するバッファをフリップ(表裏の入れ替え)
		result = swapChain->Present(1, 0);
		result = device->GetDeviceRemovedReason();
		assert(SUCCEEDED(result));
#pragma endregion

#pragma region コマンドの完了待ち
		//コマンドの実行完了を待つ
		CommandQueue->Signal(fence.Get(), fenceVal);
		if (fence->GetCompletedValue() != fenceVal) {
			HANDLE event = CreateEvent(nullptr, false, false, nullptr);
			fence->SetEventOnCompletion(fenceVal, event);
			WaitForSingleObject(event, INFINITE);
			CloseHandle(event);
		}
		//キューをクリア
		result = commandAllocator->Reset();
		assert(SUCCEEDED(result));

		//再びコマンドリストを貯める準備
		result = commandList->Reset(commandAllocator.Get(), nullptr);
		assert(SUCCEEDED(result));
#pragma endregion

#pragma endregion

	}
	//ウィンドウクラスの登録解除
	UnregisterClass(w.lpszClassName, w.hInstance);

	return 0;
}

void InitializeObject3d(Object3d* object, ID3D12Device* device)
{
	HRESULT result;

	//定数バッファのヒープ設定
	D3D12_HEAP_PROPERTIES heapProp{};
	heapProp.Type = D3D12_HEAP_TYPE_UPLOAD;
	//定数バッファのリソース設定
	D3D12_RESOURCE_DESC resdesc{};
	resdesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resdesc.Width = (sizeof(ConstBufferDataTransform) + 0xff) & ~0xff;
	resdesc.Height = 1;
	resdesc.DepthOrArraySize = 1;
	resdesc.MipLevels = 1;
	resdesc.SampleDesc.Count = 1;
	resdesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	//定数バッファの生成
	result = device->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resdesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&object->constBuffTransform)
	);
	assert(SUCCEEDED(result));

	//定数バッファのマッピング
	result = object->constBuffTransform->Map(0, nullptr, (void**)&object->constMapTransform);
	assert(SUCCEEDED(result));
}

void UpdateObject3d(Object3d* object, XMMATRIX& matView, XMMATRIX& matProjection)
{
	XMMATRIX matScale, matRot, matTrans;

	//スケール、回転、平行移動行列の計算
	matScale = XMMatrixScaling(object->scale.x, object->scale.y, object->scale.z);
	matRot = XMMatrixIdentity();
	matRot *= XMMatrixRotationZ(object->rotation.z);
	matRot *= XMMatrixRotationX(object->rotation.x);
	matRot *= XMMatrixRotationY(object->rotation.y);
	matTrans = XMMatrixTranslation(
		object->position.x, object->position.y, object->position.z);

	//ワールド行列の合成
	object->matWorld = XMMatrixIdentity();
	object->matWorld *= matScale;
	object->matWorld *= matRot;
	object->matWorld *= matTrans;

	//親子オブジェクトがあれば
	if (object->parent != nullptr) {
		object->matWorld *= object->parent->matWorld;
	}

	//定数バッファへのデータ転送
	object->constMapTransform->mat = object->matWorld * matView * matProjection;
}

void DrawObject3d(Object3d* object, ID3D12GraphicsCommandList* commandList, D3D12_VERTEX_BUFFER_VIEW& vbView,
	D3D12_INDEX_BUFFER_VIEW& ibView, UINT numIndices) {
	//頂点バッファの設定
	commandList->IASetVertexBuffers(0, 1, &vbView);
	//インデックスバッファの設定
	commandList->IASetIndexBuffer(&ibView);
	//定数バッファビューの設定コマンド
	commandList->SetGraphicsRootConstantBufferView(2, object->constBuffTransform->GetGPUVirtualAddress());

	//描画コマンド
	commandList->DrawIndexedInstanced(numIndices, 1, 0, 0, 0);
}

void MoveObject3d(Object3d* object, BYTE* key)
{
	if (key[DIK_UP] || key[DIK_DOWN] || key[DIK_LEFT] || key[DIK_RIGHT])
	{
		if (key[DIK_UP]) { object->position.y += 1.0f; }
		else if (key[DIK_DOWN]) { object->position.y -= 1.0f; }
		if (key[DIK_RIGHT]) { object->position.x += 1.0f; }
		else if (key[DIK_LEFT]) { object->position.x -= 1.0f; }
	}
}
