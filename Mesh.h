#pragma once
class Mesh
{
public:
	//コンストラクタ
	Mesh();
	//デストラクタ
	~Mesh();
	//メンバ関数
	void Initialize(ID3D12Device* device);//初期化
	void Update();
	void Draw(float x1, float y1, float x2, float y2, float x3, float y3);

	//メンバ変数

private:

};
