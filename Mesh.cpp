#include "Mesh.h"

#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include<cassert>
#include<vector>
#include<string>

#include<DirectXMath.h>
using namespace DirectX;

#include<d3dcompiler.h>
#pragma comment(lib,"d3dcompiler.lib")

#define DIRECTINPUT_VERSION 0x0800 //DirectInput�̃o�[�W�����w��
#include <dinput.h>

#pragma comment(lib,"dinput8.lib")
#pragma comment(lib,"dxguid.lib")

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
Mesh::Mesh()//�R���X�g���N�^
{
}

Mesh::~Mesh()//�f�X�g���N�^
{
}
void Mesh::Initialize(ID3D12Device* device)//������
{
	//�萔�o�b�t�@�p�f�[�^�\����(�}�e���A��)
	struct ConstBufferDataMaterial {
		XMFLOAT4 color;//�F(RGBA)
	};

	//�q�[�v�ݒ�
	D3D12_HEAP_PROPERTIES cbHeapProp{};
	cbHeapProp.Type = D3D12_HEAP_TYPE_UPLOAD;//GPU�ւ̓]��
	//���\�[�X�ݒ�
	D3D12_RESOURCE_DESC cbResourceDesc{};
	cbResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	cbResourceDesc.Width = (sizeof(ConstBufferDataMaterial) + 0xff);//256�o�C�g�A���C�����g
	cbResourceDesc.Height = 1;
	cbResourceDesc.DepthOrArraySize = 1;
	cbResourceDesc.MipLevels = 1;
	cbResourceDesc.SampleDesc.Count = 1;
	cbResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	ID3D12Resource* constBuffMaterial = nullptr;
	//�萔�o�b�t�@�̐���
	result = device->CreateCommittedResource(
		&cbHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&cbResourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&constBuffMaterial));
	assert(SUCCEEDED(result));

	//�萔�o�b�t�@�̃}�b�s���O
	ConstBufferDataMaterial* constMapMaterial = nullptr;
	result = constBuffMaterial->Map(0, nullptr, (void**)&constMapMaterial);//�}�b�s���O
	assert(SUCCEEDED(result));

	//�l���������ނƎ����I�ɓ]�������
	constMapMaterial->color = XMFLOAT4(1, 1, 1, 1.0f);   //RGBA�Ŕ������̐�

	//���[�g�p�����[�^�̐ݒ�
	D3D12_ROOT_PARAMETER rootParam = {};
	rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;	//�萔�o�b�t�@�r���[
	rootParam.Descriptor.ShaderRegister = 0;					//�萔�o�b�t�@�ԍ�
	rootParam.Descriptor.RegisterSpace = 0;						//�f�t�H���g�l
	rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;	//�S�ẴV�F�[�_���猩����

	//���_�f�[�^
	XMFLOAT3 vertices[] =
	{
		{-0.5f,-0.5f,0.0f},//�����@�C���f�b�N�X0
		{-0.5f,+0.5f,0.0f},//����@�C���f�b�N�X1
		{+0.5f,-0.5f,0.0f},//�E���@�C���f�b�N�X2
		//{+0.5f,+0.5f,0.0f},//�E��@�C���f�b�N�X3
	};
	//�C���f�b�N�X�f�[�^
	uint16_t indices[] =
	{
		0,1,2, //�O�p�`1��
		1,2,3, //�O�p�`2��
	};


	//���_�f�[�^�S�̂̃T�C�Y = ���_�f�[�^1���̃T�C�Y * ���_�f�[�^�̗v�f��
	UINT sizeVB = static_cast<UINT>(sizeof(XMFLOAT3) * _countof(vertices));

	//���_�o�b�t�@�̐ݒ�
	D3D12_HEAP_PROPERTIES heapProp{};		//�q�[�v�ݒ�
	heapProp.Type = D3D12_HEAP_TYPE_UPLOAD;	//GPU�ւ̓]���p



	//���\�[�X�ݒ� 
	D3D12_RESOURCE_DESC resDesc{};
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resDesc.Width = sizeVB;
	resDesc.Height = 1;
	resDesc.DepthOrArraySize = 1;
	resDesc.MipLevels = 1;
	resDesc.SampleDesc.Count = 1;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	//���_�o�b�t�@����
	ID3D12Resource* vertBuff = nullptr;
	result = device->CreateCommittedResource(
		&heapProp,				//�q�[�v�ݒ�
		D3D12_HEAP_FLAG_NONE,
		&resDesc,				//���\�[�X�ݒ�
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&vertBuff));
	assert(SUCCEEDED(result));

	//�C���f�b�N�X�f�[�^�S�̂̃T�C�Y
	UINT sizeIB = static_cast<UINT>(sizeof(uint16_t) * _countof(indices));

	//���\�[�X�ݒ�
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resDesc.Width = sizeIB;		//�C���f�b�N�X��񂪓��镪�̃T�C�Y
	resDesc.Height = 1;
	resDesc.DepthOrArraySize = 1;
	resDesc.MipLevels = 1;
	resDesc.SampleDesc.Count = 1;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	//�C���f�b�N�X�o�b�t�@�̐���
	ID3D12Resource* indexBuff = nullptr;
	result = device->CreateCommittedResource(
		&heapProp,		//�q�[�v�ݒ�
		D3D12_HEAP_FLAG_NONE,
		&resDesc,		//���\�[�X�ݒ�
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&indexBuff));

	//�C���f�b�N�X�o�b�t�@���}�b�s���O
	uint16_t* indexMap = nullptr;
	result = indexBuff->Map(0, nullptr, (void**)&indexMap);
	//�S�C���f�b�N�X�ɑ΂���
	for (int i = 0; i < _countof(indices); i++)
	{
		indexMap[i] = indices[i];	//�C���f�b�N�X���R�s�[
	}
	//�}�b�s���O����
	indexBuff->Unmap(0, nullptr);

	//�C���f�b�N�X�o�b�t�@�r���[�̍쐬
	D3D12_INDEX_BUFFER_VIEW ibView{};
	ibView.BufferLocation = indexBuff->GetGPUVirtualAddress();
	ibView.Format = DXGI_FORMAT_R16_UINT;
	ibView.SizeInBytes = sizeIB;

	//GPU��̃o�b�t�@�ɑΉ��������z�������i���C����������j���擾
	XMFLOAT3* vertMap = nullptr;
	result = vertBuff->Map(0, nullptr, (void**)&vertMap);
	assert(SUCCEEDED(result));
	//�S���_�ɑ΂���
	for (int i = 0; i < _countof(vertices); i++)
	{
		vertMap[i] = vertices[i]; //���W���R�s�[
	}
	//�q���������
	vertBuff->Unmap(0, nullptr);

	//���_�o�b�t�@�r���[�̍쐬
	D3D12_VERTEX_BUFFER_VIEW vbView{};
	//GPU���z�A�h���X
	vbView.BufferLocation = vertBuff->GetGPUVirtualAddress();
	//���_�o�b�t�@�̃T�C�Y
	vbView.SizeInBytes = sizeVB;
	//���_1���̃f�[�^�T�C�Y
	vbView.StrideInBytes = sizeof(XMFLOAT3);

	ID3D10Blob* vsBlob = nullptr;							//���_�V�F�[�_�I�u�W�F�N�g
	ID3D10Blob* psBlob = nullptr;					        //�s�N�Z���V�F�[�_�[�I�u�W�F�N�g
	ID3D10Blob* errorBlob = nullptr;						//�G���[�I�u�W�F�N�g

	//���_�V�F�[�_�̓ǂݍ��݂ƃR���p�C��
	result = D3DCompileFromFile(
		L"BasicVS.hlsl",									//�V�F�[�_�t�@�C����
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,					//�C���N���[�h�\�ɂ���
		"main", "vs_5_0",									//�G���^�[�|�C���g���A�V�F�[�_���f���w��
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,	//�f�o�b�N�p�ݒ�
		0, &vsBlob, &errorBlob);

	//�G���[�Ȃ�
	if (FAILED(result))
	{
		//errorBlob����G���[���e��string�^�ɃR�s�[
		std::string error;
		error.resize(errorBlob->GetBufferSize());

		std::copy_n((char*)errorBlob->GetBufferPointer(),
			errorBlob->GetBufferSize(),
			error.begin());
		error += "\n";
		//�G���[���e���o�̓E�C���h�E�ɕ\��
		OutputDebugStringA(error.c_str());
		assert(0);
	}

	//�s�N�Z���V�F�[�_�̓ǂݍ��݂ƃR���p�C��
	result = D3DCompileFromFile(
		L"BasicPS.hlsl",									//�V�F�[�_�t�@�C����
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,					//�C���N���[�h�\�ɂ���
		"main", "ps_5_0",									//�G���g���[�|�C���g���A�V�F�[�_�[���f���w��
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,	//�f�o�b�N�p�ݒ�
		0,
		&psBlob, &errorBlob);

	//�G���[�Ȃ�
	if (FAILED(result))
	{
		//errorBlob����G���[���e��string�^�ɃR�s�[
		std::string error;
		error.resize(errorBlob->GetBufferSize());

		std::copy_n((char*)errorBlob->GetBufferPointer(),
			errorBlob->GetBufferSize(),
			error.begin());
		error += "\n";
		//�G���[���e���o�̓E�B���h�E�ɕ\��
		OutputDebugStringA(error.c_str());
		assert(0);
	}

	//���_���C�A�E�g
	D3D12_INPUT_ELEMENT_DESC inputLayout[] =
	{
		{
			"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0
		},//(1�s�ŏ������ق������₷��)
	};

	//�O���t�B�b�N�X�p�C�v���C���ݒ�
	D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineDesc{};

	//�V�F�[�_�[�̐ݒ�
	pipelineDesc.VS.pShaderBytecode = vsBlob->GetBufferPointer();
	pipelineDesc.VS.BytecodeLength = vsBlob->GetBufferSize();
	pipelineDesc.PS.pShaderBytecode = psBlob->GetBufferPointer();
	pipelineDesc.PS.BytecodeLength = psBlob->GetBufferSize();

	//�T���v���}�X�N�̐ݒ�
	pipelineDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;//�W���ݒ�

	//���X�^���C�U�̐ݒ�
	pipelineDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;	//�J�����O���Ȃ�
	pipelineDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;	//�|���S�����h��Ԃ�
	//pipelineDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;	//���C���[�t���[��
	pipelineDesc.RasterizerState.DepthClipEnable = true;			//�[�x�N���b�s���O��L����

	//�u�����h�X�e�[�g
	pipelineDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL; //RGBA�S�Ẵ`�����l����o�^

	//�����_�[�^�[�Q�b�g�̃u�����h�ݒ�
	D3D12_RENDER_TARGET_BLEND_DESC& blenddesc = pipelineDesc.BlendState.RenderTarget[0];
	blenddesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;//RBGA�S�Ẵ`�����l����`��

	blenddesc.BlendEnable = true;					//�u�����h��L���ɂ���
	blenddesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;	//���Z
	blenddesc.SrcBlendAlpha = D3D12_BLEND_ONE;		//�\�[�X�̒l��100���g��
	blenddesc.DestBlendAlpha = D3D12_BLEND_ZERO;	//�f�X�g�̒l���@0���g��

	//���_���C�A�E�g�̐ݒ�
	pipelineDesc.InputLayout.pInputElementDescs = inputLayout;
	pipelineDesc.InputLayout.NumElements = _countof(inputLayout);

	//�}�`�̌`��ݒ�
	pipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	//���̑��̐ݒ�
	pipelineDesc.NumRenderTargets = 1; //�`��Ώۂ�1��
	pipelineDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;//0~255�w���RGBA
	pipelineDesc.SampleDesc.Count = 1;//1�s�N�Z���ɂ�1��T���v�����O

	//���[�g�V�O�l�N�`��
	ID3D12RootSignature* rootSignature;
	//���[�g�V�O�l�N�`���̐ݒ�
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rootSignatureDesc.pParameters = &rootParam;	//���[�g�p�����[�^�̐擪�A�h���X
	rootSignatureDesc.NumParameters = 1;		//���[�g�p�����[�^��

	//���[�g�V�O�l�N�`���̃V���A���C�Y
	ID3DBlob* rootSigBlob = nullptr;
	result = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0,
		&rootSigBlob, &errorBlob);
	assert(SUCCEEDED(result));
	result = device->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(),
		IID_PPV_ARGS(&rootSignature));
	assert(SUCCEEDED(result));
	rootSigBlob->Release();
	//�p�C�v���C���Ƀ��[�g�V�O�l�N�`�����Z�b�g
	pipelineDesc.pRootSignature = rootSignature;

	//�p�C�v�����X�e�[�g�̐���
	ID3D12PipelineState* pipelineState = nullptr;
	result = device->CreateGraphicsPipelineState(&pipelineDesc, IID_PPV_ARGS(&pipelineState));
	assert(SUCCEEDED(result));
}


//--------------------------------------------------------------------------------------------------------------------------------------------
void Mesh::Update()//�X�V
{

}


//--------------------------------------------------------------------------------------------------------------------------------------------
void Mesh::Draw(float x1, float y1, float x2, float y2, float x3, float y3)//�`��
{

}
