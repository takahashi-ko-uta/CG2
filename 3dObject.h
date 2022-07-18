#pragma once

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

#include <wrl.h>
using namespace Microsoft::WRL;

//定数バッファ用データ構造体（3D変換行列)
struct ConstBufferDataTransform {
	XMMATRIX mat; //3D変換行列
};

struct Object3d
{
	//定数バッファ（行列用）
	ID3D12Resource* constBuffTransform;
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
