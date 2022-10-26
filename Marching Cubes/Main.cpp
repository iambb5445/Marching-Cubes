#include <iostream>
#include "MarchingCubes.h"

template<int sizeX, int sizeY, int sizeZ>VolumetricData<sizeX, sizeY, sizeZ, int8> getVolumetricDataOfACube();
template<int sizeX, int sizeY, int sizeZ>VolumetricData<sizeX, sizeY, sizeZ, int8> getVolumetricDataOfARoundEdgeCube();
template<int sizeX, int sizeY>VolumetricData<sizeX, sizeY, 4, int8> getVolumetricDataOfFlatTerrain();
template<int sizeX, int sizeY>VolumetricData<sizeX, sizeY, 4, int8> getVolumetricDataOfWavedTerrain();


void main() {
	auto x = getVolumetricDataOfACube<10, 10, 10>();
}

template<int sizeX, int sizeY, int sizeZ>
VolumetricData<sizeX, sizeY, sizeZ, int8> getVolumetricDataOfACube() {
	int8 values[sizeX][sizeY][sizeZ];
	for (int i = 0; i < sizeX; i++) {
		for (int j = 0; j < sizeY; j++) {
			for (int k = 0; k < sizeZ; k++) {
				values[i][j][k] = -1;
				if (i == 0 || i == (sizeX - 1) || j == 0 || j == (sizeY - 1) || k == 0 || k == (sizeZ - 1))
					values[i][j][k] = 0;
			}
		}
	}
	return VolumetricData<sizeX, sizeY, sizeZ, int8>(values);
}

template<int sizeX, int sizeY, int sizeZ>
VolumetricData<sizeX, sizeY, sizeZ, int8> getVolumetricDataOfARoundEdgeCube() {
	int8 values[sizeX][sizeY][sizeZ];
	for (int i = 0; i < sizeX; i++) {
		for (int j = 0; j < sizeY; j++) {
			for (int k = 0; k < sizeZ; k++) {
				values[i][j][k] = -1;
				if (i == 0 || i == (sizeX - 1) || j == 0 || j == (sizeY - 1) || k == 0 || k == (sizeZ - 1))
					values[i][j][k] = 1;
			}
		}
	}
	return VolumetricData<sizeX, sizeY, sizeZ, int8>(values);
}

template<int sizeX, int sizeY>
VolumetricData<sizeX, sizeY, 4, int8> getVolumetricDataOfFlatTerrain() {

	int8 values[sizeX][sizeY][4];
	for (int i = 0; i < sizeX; i++) {
		for (int j = 0; j < sizeY; j++) {
			values[i][j][0] = -1;
			values[i][j][1] = -1;
			values[i][j][2] = 0;
			values[i][j][3] = 1;
		}
	}
	return VolumetricData<sizeX, sizeY, 4, int8>(values);
}

template<int sizeX, int sizeY>
VolumetricData<sizeX, sizeY, 4, int8> getVolumetricDataOfWavedTerrain() {

	int8 values[sizeX][sizeY][4];
	for (int i = 0; i < sizeX; i++) {
		for (int j = 0; j < sizeY; j++) {
			values[i][j][0] = -1;
			values[i][j][1] = -1;
			values[i][j][2] = -((i & 1) + 1);
			values[i][j][3] = 1;
		}
	}
	return VolumetricData<sizeX, sizeY, 4, int8>(values);
}
