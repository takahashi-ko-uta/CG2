#pragma once
class Mesh
{
public:
	//�R���X�g���N�^
	Mesh();
	//�f�X�g���N�^
	~Mesh();
	//�����o�֐�
	void Initialize(ID3D12Device* device);//������
	void Update();
	void Draw(float x1, float y1, float x2, float y2, float x3, float y3);

	//�����o�ϐ�

private:

};
