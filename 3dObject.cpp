#include "3dObject.h"

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

void InitializeObject3d(Object3d* object,ID3D12Device* device)
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

void UpdateObject3d(Object3d* object,XMMATRIX& matView,XMMATRIX& matProjection)
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
	if(key[DIK_UP] || key[DIK_DOWN] || key[DIK_LEFT] || key[DIK_RIGHT])
	{
		if (key[DIK_UP]) { object->position.y += 1.0f; }
		else if (key[DIK_DOWN]) { object->position.y -= 1.0f; }
		if (key[DIK_RIGHT]) { object->position.x += 1.0f; }
		else if (key[DIK_LEFT]) { object->position.x -= 1.0f; }
	}
}







