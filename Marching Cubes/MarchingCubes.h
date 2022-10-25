#pragma once
template<int sizeX, int sizeY, int sizeZ, class T>
class VolumetricData
{
private:
	T data[sizeX][sizeY][sizeZ];
public:
	VolumetricData(T data[sizeX][sizeY][sizeZ]);
	VolumetricData(VolumetricData<sizeX, sizeY, sizeZ, T>& volumetricData);
	~VolumetricData();
	T get(int x, int y, int z);
};