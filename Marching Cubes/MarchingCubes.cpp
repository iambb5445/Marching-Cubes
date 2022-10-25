#include "MarchingCubes.h"

template<int sizeX, int sizeY, int sizeZ, class T>
VolumetricData<sizeX, sizeY, sizeZ, T>::VolumetricData(T data[sizeX][sizeY][sizeZ])
{
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

template<int sizeX, int sizeY, int sizeZ, class T>
VolumetricData<sizeX, sizeY, sizeZ, T>::VolumetricData(VolumetricData<sizeX, sizeY, sizeZ, T>& volumetricData)
{
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

template<int sizeX, int sizeY, int sizeZ, class T>
VolumetricData<sizeX, sizeY, sizeZ, T>::~VolumetricData()
{
}

template<int sizeX, int sizeY, int sizeZ, class T>
T VolumetricData<sizeX, sizeY, sizeZ, T>::get(int x, int y, int z) {
	return this->data[x][y][z];
}