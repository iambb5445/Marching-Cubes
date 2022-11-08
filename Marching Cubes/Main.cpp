#include <iostream>
#include "MarchingCubes.h"

int get3DIndex(int x, int y, int z, int sizeX, int sizeY, int sizeZ);
VolumetricData<int8> getVolumetricDataOfACube(int sizeX, int sizeY, int sizeZ);
VolumetricData<int8> getVolumetricDataOfARoundEdgeCube(int sizeX, int sizeY, int sizeZ);
VolumetricData<int8> getVolumetricDataOfFlatTerrain(int sizeX, int sizeY);
VolumetricData<int8> getVolumetricDataOfWavedTerrain(int sizeX, int sizeY);

void main() {
	// examples of input data:
	// auto vData = getVolumetricDataOfACube(5, 5, 5);
	// auto vData = getVolumetricDataOfARoundEdgeCube(5, 5, 5);
	// auto vData = getVolumetricDataOfFlatTerrain(5, 5);
	// auto vData = getVolumetricDataOfWavedTerrain(5, 5);
	auto vData = VolumetricData<int>::fromFile("test_in.txt");
	vData.toFile("test_out.txt");
	// auto geo = MarchedGeometry(Vector3D(1, 1, 1), vData);
}

int get3DIndex(int x, int y, int z, int sizeX, int sizeY, int sizeZ) {
	return x * sizeY * sizeZ + y * sizeZ + z;
}

VolumetricData<int8> getVolumetricDataOfACube(int sizeX, int sizeY, int sizeZ) {
	int8* values = (int8*)malloc(sizeX * sizeY * sizeZ * sizeof(int8));
	for (int i = 0; i < sizeX; i++) {
		for (int j = 0; j < sizeY; j++) {
			for (int k = 0; k < sizeZ; k++) {
				values[get3DIndex(i, j, k, sizeX, sizeY, sizeZ)] = -1;
				if (i == 0 || i == (sizeX - 1) || j == 0 || j == (sizeY - 1) || k == 0 || k == (sizeZ - 1))
					values[get3DIndex(i, j, k, sizeX, sizeY, sizeZ)] = 0;
			}
		}
	}
	return VolumetricData<int8>(sizeX, sizeY, sizeZ, values);
}

VolumetricData<int8> getVolumetricDataOfARoundEdgeCube(int sizeX, int sizeY, int sizeZ) {
	int8 *values = (int8*)malloc(sizeX * sizeY * sizeZ * sizeof(int8));
	for (int i = 0; i < sizeX; i++) {
		for (int j = 0; j < sizeY; j++) {
			for (int k = 0; k < sizeZ; k++) {
				values[get3DIndex(i, j, k, sizeX, sizeY, sizeZ)] = -1;
				if (i == 0 || i == (sizeX - 1) || j == 0 || j == (sizeY - 1) || k == 0 || k == (sizeZ - 1))
					values[get3DIndex(i, j, k, sizeX, sizeY, sizeZ)] = 1;
			}
		}
	}
	return VolumetricData<int8>(sizeX, sizeY, sizeZ, values);
}

VolumetricData<int8> getVolumetricDataOfFlatTerrain(int sizeX, int sizeY) {
	int8* values = (int8*)malloc(sizeX * sizeY * 4 * sizeof(int8));
	for (int i = 0; i < sizeX; i++) {
		for (int j = 0; j < sizeY; j++) {
			values[get3DIndex(i, j, 0, sizeX, sizeY, 4)] = -1;
			values[get3DIndex(i, j, 1, sizeX, sizeY, 4)] = -1;
			values[get3DIndex(i, j, 2, sizeX, sizeY, 4)] = 0;
			values[get3DIndex(i, j, 3, sizeX, sizeY, 4)] = 1;
		}
	}
	return VolumetricData<int8>(sizeX, sizeY, 4, values);
}


VolumetricData<int8> getVolumetricDataOfWavedTerrain(int sizeX, int sizeY) {
	int8* values = (int8*)malloc(sizeX * sizeY * 4 * sizeof(int8));
	for (int i = 0; i < sizeX; i++) {
		for (int j = 0; j < sizeY; j++) {
			values[get3DIndex(i, j, 0, sizeX, sizeY, 4)] = -1;
			values[get3DIndex(i, j, 1, sizeX, sizeY, 4)] = -1;
			values[get3DIndex(i, j, 2, sizeX, sizeY, 4)] = -((i & 1) + 1);
			values[get3DIndex(i, j, 3, sizeX, sizeY, 4)] = 1;
		}
	}
	return VolumetricData<int8>(sizeX, sizeY, 4, values);
}