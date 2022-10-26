#pragma once

typedef signed char	int8;

template<int sizeX, int sizeY, int sizeZ, class T>
class VolumetricData
{
private:
	T data[sizeX][sizeY][sizeZ];
public:
	// The following function are not in MarchingCubes.cpp due to linker errors.
	// for more information refer to https://isocpp.org/wiki/faq/templates#separate-template-fn-defn-from-decl
	VolumetricData(T data[sizeX][sizeY][sizeZ]) {
		for (int i = 0; i < sizeX; i++)
		{
			for (int j = 0; j < sizeY; j++)
			{
				for (int k = 0; k < sizeZ; k++)
				{
					this->data[i][j][k] = data[i][j][k];
				}
			}
		}
	}
	VolumetricData(const VolumetricData<sizeX, sizeY, sizeZ, T>& volumetricData) {
		for (int i = 0; i < sizeX; i++)
		{
			for (int j = 0; j < sizeY; j++)
			{
				for (int k = 0; k < sizeZ; k++)
				{
					this->data[i][j][k] = volumetricData.data[i][j][k];
				}
			}
		}
	}
	T get(int x, int y, int z) {
		return this->data[x][y][z];
	}
};