#include<fstream>
#include<vector>

#pragma once

typedef signed char				int8;
typedef short					int16;
typedef int						int32;
typedef __int64					int64;
typedef unsigned char			uint8;
typedef unsigned short			uint16;
typedef unsigned int			uint32;
typedef unsigned __int64		uint64;

struct Vector3D {
	float x;
	float y;
	float z;
	Vector3D(int _x = 0, int _y = 0, int _z = 0);
	Vector3D(const Vector3D& v);
};

struct Vertex {
	Vector3D position;
};

struct Triangle {
	uint16 index[3];
};

template<typename T>
class VolumetricData
{
private:
	int sizeX, sizeY, sizeZ;
	T *data;
public:
	VolumetricData(int sizeX, int sizeY, int sizeZ, T data[]);
	VolumetricData(const VolumetricData<T>& volumetricData);
	T get(int x, int y, int z);
	int getSizeX();
	int getSizeY();
	int getSizeZ();
	~VolumetricData();
	static VolumetricData fromFile(const char filename[]);
	void toFile(const char filename[]);
};

class MarchedGeometry
{
	const static int MAX_TRIANGLE_PER_CUBE = 5;
	const static int MAX_VERTEX_PER_CUBE = 12;
	const static int CASE_COUNT = 256;
	const static int CLASS_COUNT = 16;
	const static int EDGE_COUNT = 12;
	const static int CORNER_COUNT = 8;
	struct ClassGeometry
	{
		typedef struct _GeometryCount {
			unsigned char triangleCount : 4;
			unsigned char vertexCount : 4;
		}GeometryCount;
		union {
			uint8 value;
			GeometryCount geometryCounts;
		};
		uint8 vertexIndex[MAX_TRIANGLE_PER_CUBE * 3];
	};
	struct OnEdgeVertexCode
	{
		typedef struct _CodeBits {
			unsigned char lowerNumberedCorner : 3;
			unsigned char higherNumberedCorner : 3;
			unsigned char reducedEdgeIndex : 2;
			unsigned char edgeIndex : 4;
			unsigned char edgeDelta : 4;
		}CodeBits;
		union {
			uint16 value;
			CodeBits parts;
		};
	};
	struct ReusableCubeData
	{
		const static int REUSABLE_EDGE_COUNT = 9;
		const static int REUSABLE_CORNER_COUNT = 7;
		const static uint16 BLANK = 0xFFFF;
		uint16 corner[REUSABLE_CORNER_COUNT];
		uint16 edge[REUSABLE_EDGE_COUNT];
		void reset();
		void setCorner(uint8 cornerIndex, uint16 value);
		uint16 getCorner(uint8 cornerIndex);
		void setEdge(uint8 edgeIndex, uint16 value);
		uint16 getEdge(uint8 edgeIndex);
	};
	class ReusableCubeDoubleDeck {
	private:
		int cubeCountX, cubeCountY;
		ReusableCubeData* deck[2];
	public:
		ReusableCubeDoubleDeck(int cubeCountX, int cubeCountY);
		ReusableCubeData& get(int deckId, int x, int y);
		~ReusableCubeDoubleDeck();
	};
private:
	Vector3D cubeScale;
	Vector3D size;
	VolumetricData<int8> field;

	int vertexCount;
	int triangleCount;
	Vertex *vertex;
	Triangle *triangle;
	int cubeCountX, cubeCountY, cubeCountZ;

	static const uint8 caseIndexToClassIndex[CASE_COUNT];
	static const ClassGeometry classGeometry[CLASS_COUNT];
	static const OnEdgeVertexCode onEdgeVertexCode[CASE_COUNT][MAX_VERTEX_PER_CUBE];

	int8 getCornerFieldValue(int x, int y, int z, int cornerIndex);
	uint32 getCaseIndex(int x, int y, int z);
	uint32 getCornerDeltaMask(int x, int y, int z);
	uint32 getEdgeDeltaMask(int x, int y, int z);
	int32 getInterpolationT(int x, int y, int z, OnEdgeVertexCode code);
	uint16 getVertexIndexOnCorner(int x, int y, int z, OnEdgeVertexCode code, int32 interpolationT, ReusableCubeDoubleDeck& deck);
	uint16 getNewVertexIndexOnCorner(int x, int y, int z, uint8 cornerIndex);
	uint16 getVertexIndexOnEdge(int x, int y, int z, OnEdgeVertexCode code, int32 interpolationT, ReusableCubeDoubleDeck& deck);
	uint16 getNewVertexIndexOnEdge(int x, int y, int z, OnEdgeVertexCode code, int32 interpolationT);
	bool isTriangleAreaZero(int triangleIndex);
	void setVertex(uint16 vertexIndex, float xPos, float yPos, float zPos);
	void marchCubes();
	void marchCube(int x, int y, int z, ReusableCubeDoubleDeck& deck);
public:
	MarchedGeometry(Vector3D cubeScale, VolumetricData<int8>);
	~MarchedGeometry();
	void toFile(const char filename[]);
};

// The following function are not in MarchingCubes.cpp due to linker errors.
// for more information refer to https://isocpp.org/wiki/faq/templates#separate-template-fn-defn-from-decla

template<typename T>
VolumetricData<T>::VolumetricData(int _sizeX, int _sizeY, int _sizeZ, T _data[]) {
	sizeX = _sizeX;
	sizeY = _sizeY;
	sizeZ = _sizeZ;
	data = (T*) malloc(sizeX * sizeY * sizeZ * sizeof(T));
	for (int i = 0; i < sizeX*sizeY*sizeZ; i++) {
		data[i] = _data[i];
	}
}

template<typename T>
VolumetricData<T>::VolumetricData(const VolumetricData<T>& volumetricData) {
	sizeX = volumetricData.sizeX;
	sizeY = volumetricData.sizeY;
	sizeZ = volumetricData.sizeZ;
	data = (T*)malloc(sizeX * sizeY * sizeZ * sizeof(T));
	for (int i = 0; i < sizeX * sizeY * sizeZ; i++) {
		data[i] = volumetricData.data[i];
	}
}

template<typename T>
T VolumetricData<T>::get(int x, int y, int z) {
	if (x < 0 || x >= sizeX) {
		throw std::runtime_error("X dimention out of bound in VolumetricData get function");
	}
	if (y < 0 || y >= sizeY) {
		throw std::runtime_error("Y dimention out of bound in VolumetricData get function");
	}
	if (z < 0 || z >= sizeZ) {
		throw std::runtime_error("Z dimention out of bound in VolumetricData get function");
	}
	return data[x * sizeY * sizeZ + y * sizeZ + z];
}

template<typename T>
VolumetricData<T>::~VolumetricData() {
	free(data);
}

template<typename T>
int VolumetricData<T>::getSizeX() {
	return sizeX;
}

template<typename T>
int VolumetricData<T>::getSizeY() {
	return sizeY;
}

template<typename T>
int VolumetricData<T>::getSizeZ() {
	return sizeZ;
}

template<typename T>
VolumetricData<T> VolumetricData<T>::fromFile(const char filename[]) {
	std::ifstream fin(filename);
	if (!fin.is_open()) {
;		throw std::runtime_error("Can't open file");
	}
	int sizeX, sizeY, sizeZ;
	fin >> sizeX >> sizeY >> sizeZ;
	T* data = (T*)malloc(sizeX*sizeY*sizeZ*sizeof(T));
	for (int i = 0; i < sizeX * sizeY * sizeZ; i++) {
		int value;
		fin >> value;
		data[i] = (T)value; // TODO proper casting
	}
	fin.close();
	auto ret = VolumetricData<T>(sizeX, sizeY, sizeZ, data);
	free(data);
	return ret;
}

template<typename T>
void VolumetricData<T>::toFile(const char filename[]) {
	std::ofstream fout(filename);
	if (!fout.is_open()) {
		throw std::runtime_error("Can't open file");
	}
	fout << sizeX << " " << sizeY << " " << sizeZ << std::endl;
	for (int i = 0; i < sizeX * sizeY * sizeZ; i++) {
		if (i > 0) {
			fout << " ";
		}
		fout << (int)data[i];
	}
	fout.close();
}