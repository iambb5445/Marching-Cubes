#include "MarchingCubes.h"

Vector3D::Vector3D(int _x, int _y, int _z) {
	this->x = _x;
	this->y = _y;
	this->z = _z;
}

Vector3D::Vector3D(const Vector3D& v) {
	this->x = v.x;
	this->y = v.y;
	this->z = v.z;
}

MarchedGeometry::MarchedGeometry(Vector3D cubeScale, VolumetricData<int8>_field) : field(_field)
{
	this->cubeScale = cubeScale;
	cubeCountX = field.getSizeX() - 1;
	cubeCountY = field.getSizeY() - 1;
	cubeCountZ = field.getSizeZ() - 1;
	this->size = Vector3D(cubeScale.x * cubeCountX, cubeScale.y * cubeCountY, cubeScale.z * cubeCountZ);
	vertexCount = 0;
	triangleCount = 0;
	int cubeCount = cubeCountX * cubeCountY * cubeCountZ;
	int maxVertexCount = cubeCount * MAX_VERTEX_PER_CUBE;
	int maxTriangleCount = cubeCount * MAX_TRIANGLE_PER_CUBE;
	vertex = (Vertex*)malloc(maxVertexCount * sizeof(Vertex));
	triangle = (Triangle*)malloc(maxTriangleCount * sizeof(Triangle));
	marchCubes();
}

MarchedGeometry::~MarchedGeometry()
{
	free(vertex);
	free(triangle);
}

int8 MarchedGeometry::getCornerFieldValue(int x, int y, int z, int cornerIndex) {
	return field.get(x + (cornerIndex & 1), y + ((cornerIndex & 2) >> 1), z + ((cornerIndex & 4) >> 2));
}

uint32 MarchedGeometry::getCaseIndex(int x, int y, int z)
{
	uint32 value = 0;
	for (int i = 0; i < CORNER_COUNT; i++) {
		int8 vertexValue = getCornerFieldValue(x, y, z, i);
		uint32 vertexSign = vertexValue & 0x80;
		value |= vertexSign >> (7 - i);
	}
	return value;
}

uint32 MarchedGeometry::getCornerDeltaMask(int x, int y, int z)
{
	uint32 xMask = (x == 0) ? 0 : 1;
	uint32 yMask = (y == 0) ? 0 : 2;
	uint32 zMask = (z == 0) ? 0 : 4;
	return (xMask | yMask | zMask);
}

uint32 MarchedGeometry::getEdgeDeltaMask(int x, int y, int z)
{
	uint32 xMask = (x == 0) ? 0 : 1;
	uint32 yMask = (y == 0) ? 0 : 6;
	uint32 zMask = (z == 0) ? 0 : 8;
	return (xMask | yMask | zMask);
}

int32 MarchedGeometry::getInterpolationT(int x, int y, int z, OnEdgeVertexCode code)
{
	int32 fieldValue0 = getCornerFieldValue(x, y, z, code.parts.lowerNumberedCorner);
	int32 fieldValue1 = getCornerFieldValue(x, y, z, code.parts.higherNumberedCorner);
	int32 interpolationT = (fieldValue1 << 8) / (fieldValue1 - fieldValue0);
	return interpolationT;
}

void MarchedGeometry::ReusableCubeData::reset() {
	for (int i = 0; i < REUSABLE_CORNER_COUNT; i++) {
		corner[i] = BLANK;
	}
	for (int i = 0; i < REUSABLE_EDGE_COUNT; i++) {
		edge[i] = BLANK;
	}
}

void MarchedGeometry::ReusableCubeData::setCorner(uint8 cornerIndex, uint16 value) {
	if (cornerIndex >= (CORNER_COUNT - REUSABLE_CORNER_COUNT)) {
		corner[cornerIndex - (CORNER_COUNT - REUSABLE_CORNER_COUNT)] = value;
	}
}

uint16 MarchedGeometry::ReusableCubeData::getCorner(uint8 cornerIndex) {
	if (cornerIndex >= (CORNER_COUNT - REUSABLE_CORNER_COUNT)) {
		return corner[cornerIndex - (CORNER_COUNT - REUSABLE_CORNER_COUNT)];
	}
	return BLANK;
}

void MarchedGeometry::ReusableCubeData::setEdge(uint8 edgeIndex, uint16 value) {
	if (edgeIndex >= (EDGE_COUNT - REUSABLE_EDGE_COUNT)) {
		edge[edgeIndex - (EDGE_COUNT - REUSABLE_EDGE_COUNT)] = value;
	}
}

uint16 MarchedGeometry::ReusableCubeData::getEdge(uint8 edgeIndex) {
	if (edgeIndex >= (EDGE_COUNT - REUSABLE_EDGE_COUNT)) {
		return edge[edgeIndex - (EDGE_COUNT - REUSABLE_EDGE_COUNT)];
	}
	return BLANK;
}

MarchedGeometry::ReusableCubeDoubleDeck::ReusableCubeDoubleDeck(int _cubeCountX, int _cubeCountY) {
	cubeCountX = _cubeCountX;
	cubeCountY = _cubeCountY;
	deck[0] = (ReusableCubeData*)malloc(cubeCountX * cubeCountY * sizeof(ReusableCubeData));
	deck[1] = (ReusableCubeData*)malloc(cubeCountX * cubeCountY * sizeof(ReusableCubeData));
}

MarchedGeometry::ReusableCubeData& MarchedGeometry::ReusableCubeDoubleDeck::get(int deckId, int x, int y) {
	return deck[deckId][x * cubeCountY + y];
}

MarchedGeometry::ReusableCubeDoubleDeck::~ReusableCubeDoubleDeck() {
	free(deck[0]);
	free(deck[1]);
}

void MarchedGeometry::setVertex(uint16 vertexIndex, float xPos, float yPos, float zPos)
{
	vertex[vertexIndex].position.x = xPos;
	vertex[vertexIndex].position.y = yPos;
	vertex[vertexIndex].position.z = zPos;
	// TODO set normal, tangent, and texcoord
}

bool MarchedGeometry::isTriangleAreaZero(int triangleIndex)
{
	if (triangle[triangleIndex].index[0] == triangle[triangleIndex].index[1]) {
		return true;
	}
	if (triangle[triangleIndex].index[0] == triangle[triangleIndex].index[2]) {
		return true;
	}
	if (triangle[triangleIndex].index[1] == triangle[triangleIndex].index[2]) {
		return true;
	}
	return false;
}

uint16 MarchedGeometry::getNewVertexIndexOnCorner(int x, int y, int z, uint8 cornerIndex) {
	float xPos = cubeScale.x * (x + ((cornerIndex >> 0) & 1));
	float yPos = cubeScale.y * (y + ((cornerIndex >> 1) & 1));
	float zPos = cubeScale.z * (z + ((cornerIndex >> 2) & 1));
	setVertex(vertexCount, xPos, yPos, zPos);
	return vertexCount++;
}

uint16 MarchedGeometry::getVertexIndexOnCorner(int x, int y, int z, OnEdgeVertexCode code, int32 interpolationT, ReusableCubeDoubleDeck& deck) {
	uint32 deltaMask = getCornerDeltaMask(x, y, z);
	uint8 cornerIndex = ((interpolationT == 0) ? code.parts.lowerNumberedCorner : code.parts.higherNumberedCorner);
	uint16 delta = cornerIndex ^ 7;
	uint16 maskedDelta = delta & deltaMask;
	cornerIndex += maskedDelta;
	x -= ((maskedDelta >> 0) & 1);
	y -= ((maskedDelta >> 1) & 1);
	z -= ((maskedDelta >> 2) & 1);
	uint16 vertexIndex = deck.get(z & 1, x, y).getCorner(cornerIndex);
	if (vertexIndex == ReusableCubeData::BLANK) {
		vertexIndex = getNewVertexIndexOnCorner(x, y, z, cornerIndex);
		deck.get(z & 1, x, y).setCorner(cornerIndex, vertexIndex);
	}
	return vertexIndex;
}

uint16 MarchedGeometry::getNewVertexIndexOnEdge(int x, int y, int z, OnEdgeVertexCode code, int32 interpolationT) {
	float interpolatedX = (((code.parts.lowerNumberedCorner >> 0) & 1) * interpolationT + ((code.parts.higherNumberedCorner >> 0) & 1) * (0x0100 - interpolationT)) / 256.0;
	float interpolatedY = (((code.parts.lowerNumberedCorner >> 1) & 1) * interpolationT + ((code.parts.higherNumberedCorner >> 1) & 1) * (0x0100 - interpolationT)) / 256.0;
	float interpolatedZ = (((code.parts.lowerNumberedCorner >> 2) & 1) * interpolationT + ((code.parts.higherNumberedCorner >> 2) & 1) * (0x0100 - interpolationT)) / 256.0;
	float xPos = cubeScale.x * (x + interpolatedX);
	float yPos = cubeScale.y * (y + interpolatedY);
	float zPos = cubeScale.z * (z + interpolatedZ);
	setVertex(vertexCount, xPos, yPos, zPos);
	return 	vertexCount++;
}

uint16 MarchedGeometry::getVertexIndexOnEdge(int x, int y, int z, OnEdgeVertexCode code, int32 interpolationT, ReusableCubeDoubleDeck& deck) {
	uint32 deltaMask = getEdgeDeltaMask(x, y, z);
	uint32 edgeDelta = code.parts.edgeDelta;
	uint16 edgeIndex = code.parts.edgeIndex;
	uint16 maskedDelta = edgeDelta & deltaMask;
	edgeIndex += (maskedDelta & 1) * 3 + ((maskedDelta >> 1) & 1) * 3 + ((maskedDelta >> 2) & 1) * 3 + ((maskedDelta >> 3) & 1) * 6;
	x -= ((maskedDelta >> 0) & 1);
	y -= ((maskedDelta >> 1) & 1);
	z -= ((maskedDelta >> 3) & 1);
	uint16 vertexIndex = deck.get(z & 1, x, y).getEdge(edgeIndex);
	if (vertexIndex == ReusableCubeData::BLANK) {
		vertexIndex = getNewVertexIndexOnEdge(x, y, z, code, interpolationT);
		deck.get(z & 1, x, y).setEdge(edgeIndex, vertexIndex);
	}
	return vertexIndex;
}

void MarchedGeometry::marchCube(int x, int y, int z, ReusableCubeDoubleDeck& deck)
{
	deck.get(z & 1, x, y).reset();
	uint16 cubeVertexIndex[MAX_VERTEX_PER_CUBE];
	uint32 caseIndex = this->getCaseIndex(x, y, z);
	uint8 classIndex = caseIndexToClassIndex[caseIndex];
	ClassGeometry geometry = classGeometry[classIndex];
	for (int i = 0; i < geometry.geometryCounts.vertexCount; i++) {
		OnEdgeVertexCode code = onEdgeVertexCode[caseIndex][i];
		int32 interpolationT = getInterpolationT(x, y, z, code);
		if (interpolationT == 0 || interpolationT == 0x0100) {
			cubeVertexIndex[i] = getVertexIndexOnCorner(x, y, z, code, interpolationT, deck);
		}
		else {
			cubeVertexIndex[i] = getVertexIndexOnEdge(x, y, z, code, interpolationT, deck);
		}
	}
	for (int i = 0; i < geometry.geometryCounts.triangleCount; i++) {
		for (int j = 0; j < 3; j++) {
			triangle[triangleCount].index[j] = cubeVertexIndex[geometry.vertexIndex[3 * i + j]];
		}
		if (!isTriangleAreaZero(triangleCount)) {
			triangleCount++;
		}
	}
}

void MarchedGeometry::marchCubes()
{
	ReusableCubeDoubleDeck reusableCubeDoubleDeck = ReusableCubeDoubleDeck(cubeCountX, cubeCountY);
	for (int k = 0; k < cubeCountZ; k++) {
		for (int j = 0; j < cubeCountY; j++) {
			for (int i = 0; i < cubeCountX; i++) {
				marchCube(i, j, k, reusableCubeDoubleDeck);
			}
		}
	}
}

void MarchedGeometry::toFile(const char filename[]) {
	std::ofstream fout(filename);
	if (!fout.is_open()) {
		throw std::runtime_error("Can't open file");
	}
	fout << vertexCount << std::endl;
	for (int i = 0; i < vertexCount; i++) {
		fout << vertex[i].position.x << " " << vertex[i].position.y << " " << vertex[i].position.z << std::endl;
	}
	fout << triangleCount << std::endl;
	for (int i = 0; i < triangleCount; i++) {
		fout << triangle[i].index[0] << " " << triangle[i].index[1] << " " << triangle[i].index[2] << std::endl;
	}
	fout.close();
}

const uint8 MarchedGeometry::caseIndexToClassIndex[MarchedGeometry::CASE_COUNT] = {
	0x00, 0x01, 0x01, 0x03, 0x01, 0x03, 0x02, 0x04, 0x01, 0x02, 0x03, 0x04, 0x03, 0x04, 0x04, 0x03,
	0x01, 0x03, 0x02, 0x04, 0x02, 0x04, 0x06, 0x0C, 0x02, 0x05, 0x05, 0x0B, 0x05, 0x0A, 0x07, 0x04,
	0x01, 0x02, 0x03, 0x04, 0x02, 0x05, 0x05, 0x0A, 0x02, 0x06, 0x04, 0x0C, 0x05, 0x07, 0x0B, 0x04,
	0x03, 0x04, 0x04, 0x03, 0x05, 0x0B, 0x07, 0x04, 0x05, 0x07, 0x0A, 0x04, 0x08, 0x0E, 0x0E, 0x03,
	0x01, 0x02, 0x02, 0x05, 0x03, 0x04, 0x05, 0x0B, 0x02, 0x06, 0x05, 0x07, 0x04, 0x0C, 0x0A, 0x04,
	0x03, 0x04, 0x05, 0x0A, 0x04, 0x03, 0x07, 0x04, 0x05, 0x07, 0x08, 0x0E, 0x0B, 0x04, 0x0E, 0x03,
	0x02, 0x06, 0x05, 0x07, 0x05, 0x07, 0x08, 0x0E, 0x06, 0x09, 0x07, 0x0F, 0x07, 0x0F, 0x0E, 0x0D,
	0x04, 0x0C, 0x0B, 0x04, 0x0A, 0x04, 0x0E, 0x03, 0x07, 0x0F, 0x0E, 0x0D, 0x0E, 0x0D, 0x02, 0x01,
	0x01, 0x02, 0x02, 0x05, 0x02, 0x05, 0x06, 0x07, 0x03, 0x05, 0x04, 0x0A, 0x04, 0x0B, 0x0C, 0x04,
	0x02, 0x05, 0x06, 0x07, 0x06, 0x07, 0x09, 0x0F, 0x05, 0x08, 0x07, 0x0E, 0x07, 0x0E, 0x0F, 0x0D,
	0x03, 0x05, 0x04, 0x0B, 0x05, 0x08, 0x07, 0x0E, 0x04, 0x07, 0x03, 0x04, 0x0A, 0x0E, 0x04, 0x03,
	0x04, 0x0A, 0x0C, 0x04, 0x07, 0x0E, 0x0F, 0x0D, 0x0B, 0x0E, 0x04, 0x03, 0x0E, 0x02, 0x0D, 0x01,
	0x03, 0x05, 0x05, 0x08, 0x04, 0x0A, 0x07, 0x0E, 0x04, 0x07, 0x0B, 0x0E, 0x03, 0x04, 0x04, 0x03,
	0x04, 0x0B, 0x07, 0x0E, 0x0C, 0x04, 0x0F, 0x0D, 0x0A, 0x0E, 0x0E, 0x02, 0x04, 0x03, 0x0D, 0x01,
	0x04, 0x07, 0x0A, 0x0E, 0x0B, 0x0E, 0x0E, 0x02, 0x0C, 0x0F, 0x04, 0x0D, 0x04, 0x0D, 0x03, 0x01,
	0x03, 0x04, 0x04, 0x03, 0x04, 0x03, 0x0D, 0x01, 0x04, 0x0D, 0x03, 0x01, 0x03, 0x01, 0x01, 0x00
};

const typename MarchedGeometry::ClassGeometry MarchedGeometry::classGeometry[MarchedGeometry::CLASS_COUNT] = {
	{0x00, {}},
	{0x31, {0, 1, 2}},
	{0x62, {0, 1, 2, 3, 4, 5}},
	{0x42, {0, 1, 2, 0, 2, 3}},
	{0x53, {0, 1, 4, 1, 3, 4, 1, 2, 3}},
	{0x73, {0, 1, 2, 0, 2, 3, 4, 5, 6}},
	{0x93, {0, 1, 2, 3, 4, 5, 6, 7, 8}},
	{0x84, {0, 1, 4, 1, 3, 4, 1, 2, 3, 5, 6, 7}},
	{0x84, {0, 1, 2, 0, 2, 3, 4, 5, 6, 4, 6, 7}},
	{0xC4, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}},
	{0x64, {0, 4, 5, 0, 1, 4, 1, 3, 4, 1, 2, 3}},
	{0x64, {0, 5, 4, 0, 4, 1, 1, 4, 3, 1, 3, 2}},
	{0x64, {0, 4, 5, 0, 3, 4, 0, 1, 3, 1, 2, 3}},
	{0x64, {0, 1, 2, 0, 2, 3, 0, 3, 4, 0, 4, 5}},
	{0x75, {0, 1, 2, 0, 2, 3, 0, 3, 4, 0, 4, 5, 0, 5, 6}},
	{0x95, {0, 4, 5, 0, 3, 4, 0, 1, 3, 1, 2, 3, 6, 7, 8}}
};

const typename MarchedGeometry::OnEdgeVertexCode MarchedGeometry::onEdgeVertexCode[MarchedGeometry::CASE_COUNT][MarchedGeometry::MAX_VERTEX_PER_CUBE] = {
	{},
	{0xA188, 0x9050, 0x72E0},
	{0xA188, 0x65E9, 0x8359},
	{0x9050, 0x72E0, 0x65E9, 0x8359},
	{0x9050, 0x849A, 0x18F2},
	{0x72E0, 0xA188, 0x849A, 0x18F2},
	{0xA188, 0x65E9, 0x8359, 0x9050, 0x849A, 0x18F2},
	{0x849A, 0x18F2, 0x72E0, 0x65E9, 0x8359},
	{0x8359, 0x0BFB, 0x849A},
	{0xA188, 0x9050, 0x72E0, 0x849A, 0x8359, 0x0BFB},
	{0xA188, 0x65E9, 0x0BFB, 0x849A},
	{0x9050, 0x72E0, 0x65E9, 0x0BFB, 0x849A},
	{0x9050, 0x8359, 0x0BFB, 0x18F2},
	{0x8359, 0x0BFB, 0x18F2, 0x72E0, 0xA188},
	{0xA188, 0x65E9, 0x0BFB, 0x18F2, 0x9050},
	{0x72E0, 0x65E9, 0x0BFB, 0x18F2},
	{0x72E0, 0x1674, 0x27AC},
	{0xA188, 0x9050, 0x1674, 0x27AC},
	{0xA188, 0x65E9, 0x8359, 0x72E0, 0x1674, 0x27AC},
	{0x65E9, 0x8359, 0x9050, 0x1674, 0x27AC},
	{0x9050, 0x849A, 0x18F2, 0x72E0, 0x1674, 0x27AC},
	{0x1674, 0x27AC, 0xA188, 0x849A, 0x18F2},
	{0x72E0, 0x1674, 0x27AC, 0xA188, 0x65E9, 0x8359, 0x9050, 0x849A, 0x18F2},
	{0x849A, 0x18F2, 0x1674, 0x27AC, 0x65E9, 0x8359},
	{0x849A, 0x8359, 0x0BFB, 0x72E0, 0x1674, 0x27AC},
	{0xA188, 0x9050, 0x1674, 0x27AC, 0x849A, 0x8359, 0x0BFB},
	{0x849A, 0xA188, 0x65E9, 0x0BFB, 0x72E0, 0x1674, 0x27AC},
	{0x849A, 0x0BFB, 0x65E9, 0x27AC, 0x1674, 0x9050},
	{0x9050, 0x8359, 0x0BFB, 0x18F2, 0x72E0, 0x1674, 0x27AC},
	{0x8359, 0x0BFB, 0x18F2, 0x1674, 0x27AC, 0xA188},
	{0xA188, 0x65E9, 0x0BFB, 0x18F2, 0x9050, 0x72E0, 0x1674, 0x27AC},
	{0x27AC, 0x65E9, 0x0BFB, 0x18F2, 0x1674},
	{0x65E9, 0x27AC, 0x097D},
	{0xA188, 0x9050, 0x72E0, 0x65E9, 0x27AC, 0x097D},
	{0x8359, 0xA188, 0x27AC, 0x097D},
	{0x27AC, 0x097D, 0x8359, 0x9050, 0x72E0},
	{0x9050, 0x849A, 0x18F2, 0x65E9, 0x27AC, 0x097D},
	{0xA188, 0x849A, 0x18F2, 0x72E0, 0x65E9, 0x27AC, 0x097D},
	{0xA188, 0x27AC, 0x097D, 0x8359, 0x9050, 0x849A, 0x18F2},
	{0x849A, 0x18F2, 0x72E0, 0x27AC, 0x097D, 0x8359},
	{0x849A, 0x8359, 0x0BFB, 0x65E9, 0x27AC, 0x097D},
	{0xA188, 0x9050, 0x72E0, 0x849A, 0x8359, 0x0BFB, 0x65E9, 0x27AC, 0x097D},
	{0x0BFB, 0x849A, 0xA188, 0x27AC, 0x097D},
	{0x9050, 0x72E0, 0x27AC, 0x097D, 0x0BFB, 0x849A},
	{0x9050, 0x8359, 0x0BFB, 0x18F2, 0x65E9, 0x27AC, 0x097D},
	{0x8359, 0x0BFB, 0x18F2, 0x72E0, 0xA188, 0x65E9, 0x27AC, 0x097D},
	{0x9050, 0x18F2, 0x0BFB, 0x097D, 0x27AC, 0xA188},
	{0x097D, 0x0BFB, 0x18F2, 0x72E0, 0x27AC},
	{0x65E9, 0x72E0, 0x1674, 0x097D},
	{0xA188, 0x9050, 0x1674, 0x097D, 0x65E9},
	{0x72E0, 0x1674, 0x097D, 0x8359, 0xA188},
	{0x8359, 0x9050, 0x1674, 0x097D},
	{0x65E9, 0x72E0, 0x1674, 0x097D, 0x9050, 0x849A, 0x18F2},
	{0x18F2, 0x849A, 0xA188, 0x65E9, 0x097D, 0x1674},
	{0x72E0, 0x1674, 0x097D, 0x8359, 0xA188, 0x9050, 0x849A, 0x18F2},
	{0x18F2, 0x1674, 0x097D, 0x8359, 0x849A},
	{0x65E9, 0x72E0, 0x1674, 0x097D, 0x849A, 0x8359, 0x0BFB},
	{0xA188, 0x9050, 0x1674, 0x097D, 0x65E9, 0x849A, 0x8359, 0x0BFB},
	{0x72E0, 0x1674, 0x097D, 0x0BFB, 0x849A, 0xA188},
	{0x849A, 0x9050, 0x1674, 0x097D, 0x0BFB},
	{0x65E9, 0x72E0, 0x1674, 0x097D, 0x9050, 0x8359, 0x0BFB, 0x18F2},
	{0xA188, 0x8359, 0x0BFB, 0x18F2, 0x1674, 0x097D, 0x65E9},
	{0xA188, 0x72E0, 0x1674, 0x097D, 0x0BFB, 0x18F2, 0x9050},
	{0x18F2, 0x1674, 0x097D, 0x0BFB},
	{0x18F2, 0x0ABE, 0x1674},
	{0xA188, 0x9050, 0x72E0, 0x18F2, 0x0ABE, 0x1674},
	{0xA188, 0x65E9, 0x8359, 0x18F2, 0x0ABE, 0x1674},
	{0x9050, 0x72E0, 0x65E9, 0x8359, 0x18F2, 0x0ABE, 0x1674},
	{0x9050, 0x849A, 0x0ABE, 0x1674},
	{0x72E0, 0xA188, 0x849A, 0x0ABE, 0x1674},
	{0x9050, 0x849A, 0x0ABE, 0x1674, 0xA188, 0x65E9, 0x8359},
	{0x1674, 0x0ABE, 0x849A, 0x8359, 0x65E9, 0x72E0},
	{0x8359, 0x0BFB, 0x849A, 0x18F2, 0x0ABE, 0x1674},
	{0xA188, 0x9050, 0x72E0, 0x849A, 0x8359, 0x0BFB, 0x18F2, 0x0ABE, 0x1674},
	{0xA188, 0x65E9, 0x0BFB, 0x849A, 0x18F2, 0x0ABE, 0x1674},
	{0x9050, 0x72E0, 0x65E9, 0x0BFB, 0x849A, 0x18F2, 0x0ABE, 0x1674},
	{0x0ABE, 0x1674, 0x9050, 0x8359, 0x0BFB},
	{0xA188, 0x8359, 0x0BFB, 0x0ABE, 0x1674, 0x72E0},
	{0xA188, 0x65E9, 0x0BFB, 0x0ABE, 0x1674, 0x9050},
	{0x1674, 0x72E0, 0x65E9, 0x0BFB, 0x0ABE},
	{0x72E0, 0x18F2, 0x0ABE, 0x27AC},
	{0x18F2, 0x0ABE, 0x27AC, 0xA188, 0x9050},
	{0x72E0, 0x18F2, 0x0ABE, 0x27AC, 0xA188, 0x65E9, 0x8359},
	{0x18F2, 0x0ABE, 0x27AC, 0x65E9, 0x8359, 0x9050},
	{0x9050, 0x849A, 0x0ABE, 0x27AC, 0x72E0},
	{0xA188, 0x849A, 0x0ABE, 0x27AC},
	{0x9050, 0x849A, 0x0ABE, 0x27AC, 0x72E0, 0xA188, 0x65E9, 0x8359},
	{0x8359, 0x849A, 0x0ABE, 0x27AC, 0x65E9},
	{0x72E0, 0x18F2, 0x0ABE, 0x27AC, 0x849A, 0x8359, 0x0BFB},
	{0x18F2, 0x0ABE, 0x27AC, 0xA188, 0x9050, 0x849A, 0x8359, 0x0BFB},
	{0x72E0, 0x18F2, 0x0ABE, 0x27AC, 0x849A, 0xA188, 0x65E9, 0x0BFB},
	{0x9050, 0x18F2, 0x0ABE, 0x27AC, 0x65E9, 0x0BFB, 0x849A},
	{0x72E0, 0x27AC, 0x0ABE, 0x0BFB, 0x8359, 0x9050},
	{0x0BFB, 0x0ABE, 0x27AC, 0xA188, 0x8359},
	{0x9050, 0xA188, 0x65E9, 0x0BFB, 0x0ABE, 0x27AC, 0x72E0},
	{0x65E9, 0x0BFB, 0x0ABE, 0x27AC},
	{0x65E9, 0x27AC, 0x097D, 0x18F2, 0x0ABE, 0x1674},
	{0xA188, 0x9050, 0x72E0, 0x65E9, 0x27AC, 0x097D, 0x18F2, 0x0ABE, 0x1674},
	{0xA188, 0x27AC, 0x097D, 0x8359, 0x18F2, 0x0ABE, 0x1674},
	{0x27AC, 0x097D, 0x8359, 0x9050, 0x72E0, 0x18F2, 0x0ABE, 0x1674},
	{0x849A, 0x0ABE, 0x1674, 0x9050, 0x65E9, 0x27AC, 0x097D},
	{0x72E0, 0xA188, 0x849A, 0x0ABE, 0x1674, 0x65E9, 0x27AC, 0x097D},
	{0x849A, 0x0ABE, 0x1674, 0x9050, 0xA188, 0x27AC, 0x097D, 0x8359},
	{0x72E0, 0x27AC, 0x097D, 0x8359, 0x849A, 0x0ABE, 0x1674},
	{0x849A, 0x8359, 0x0BFB, 0x65E9, 0x27AC, 0x097D, 0x18F2, 0x0ABE, 0x1674},
	{0xA188, 0x9050, 0x72E0, 0x849A, 0x8359, 0x0BFB, 0x65E9, 0x27AC, 0x097D, 0x18F2, 0x0ABE, 0x1674},
	{0x0BFB, 0x849A, 0xA188, 0x27AC, 0x097D, 0x18F2, 0x0ABE, 0x1674},
	{0x849A, 0x9050, 0x72E0, 0x27AC, 0x097D, 0x0BFB, 0x18F2, 0x0ABE, 0x1674},
	{0x0ABE, 0x1674, 0x9050, 0x8359, 0x0BFB, 0x65E9, 0x27AC, 0x097D},
	{0xA188, 0x8359, 0x0BFB, 0x0ABE, 0x1674, 0x72E0, 0x65E9, 0x27AC, 0x097D},
	{0x0BFB, 0x0ABE, 0x1674, 0x9050, 0xA188, 0x27AC, 0x097D},
	{0x72E0, 0x27AC, 0x097D, 0x0BFB, 0x0ABE, 0x1674},
	{0x097D, 0x65E9, 0x72E0, 0x18F2, 0x0ABE},
	{0x0ABE, 0x097D, 0x65E9, 0xA188, 0x9050, 0x18F2},
	{0x0ABE, 0x18F2, 0x72E0, 0xA188, 0x8359, 0x097D},
	{0x0ABE, 0x097D, 0x8359, 0x9050, 0x18F2},
	{0x9050, 0x849A, 0x0ABE, 0x097D, 0x65E9, 0x72E0},
	{0x65E9, 0xA188, 0x849A, 0x0ABE, 0x097D},
	{0x72E0, 0x9050, 0x849A, 0x0ABE, 0x097D, 0x8359, 0xA188},
	{0x8359, 0x849A, 0x0ABE, 0x097D},
	{0x097D, 0x65E9, 0x72E0, 0x18F2, 0x0ABE, 0x849A, 0x8359, 0x0BFB},
	{0x097D, 0x65E9, 0xA188, 0x9050, 0x18F2, 0x0ABE, 0x849A, 0x8359, 0x0BFB},
	{0x097D, 0x0BFB, 0x849A, 0xA188, 0x72E0, 0x18F2, 0x0ABE},
	{0x9050, 0x18F2, 0x0ABE, 0x097D, 0x0BFB, 0x849A},
	{0x0ABE, 0x097D, 0x65E9, 0x72E0, 0x9050, 0x8359, 0x0BFB},
	{0xA188, 0x8359, 0x0BFB, 0x0ABE, 0x097D, 0x65E9},
	{0xA188, 0x72E0, 0x9050, 0x0BFB, 0x0ABE, 0x097D},
	{0x0BFB, 0x0ABE, 0x097D},
	{0x0BFB, 0x097D, 0x0ABE},
	{0xA188, 0x9050, 0x72E0, 0x0BFB, 0x097D, 0x0ABE},
	{0xA188, 0x65E9, 0x8359, 0x0BFB, 0x097D, 0x0ABE},
	{0x9050, 0x72E0, 0x65E9, 0x8359, 0x0BFB, 0x097D, 0x0ABE},
	{0x9050, 0x849A, 0x18F2, 0x0BFB, 0x097D, 0x0ABE},
	{0xA188, 0x849A, 0x18F2, 0x72E0, 0x0BFB, 0x097D, 0x0ABE},
	{0xA188, 0x65E9, 0x8359, 0x9050, 0x849A, 0x18F2, 0x0BFB, 0x097D, 0x0ABE},
	{0x849A, 0x18F2, 0x72E0, 0x65E9, 0x8359, 0x0BFB, 0x097D, 0x0ABE},
	{0x8359, 0x097D, 0x0ABE, 0x849A},
	{0x849A, 0x8359, 0x097D, 0x0ABE, 0xA188, 0x9050, 0x72E0},
	{0x097D, 0x0ABE, 0x849A, 0xA188, 0x65E9},
	{0x72E0, 0x65E9, 0x097D, 0x0ABE, 0x849A, 0x9050},
	{0x18F2, 0x9050, 0x8359, 0x097D, 0x0ABE},
	{0x097D, 0x8359, 0xA188, 0x72E0, 0x18F2, 0x0ABE},
	{0x18F2, 0x9050, 0xA188, 0x65E9, 0x097D, 0x0ABE},
	{0x0ABE, 0x18F2, 0x72E0, 0x65E9, 0x097D},
	{0x72E0, 0x1674, 0x27AC, 0x0BFB, 0x097D, 0x0ABE},
	{0xA188, 0x9050, 0x1674, 0x27AC, 0x0BFB, 0x097D, 0x0ABE},
	{0xA188, 0x65E9, 0x8359, 0x72E0, 0x1674, 0x27AC, 0x0BFB, 0x097D, 0x0ABE},
	{0x65E9, 0x8359, 0x9050, 0x1674, 0x27AC, 0x0BFB, 0x097D, 0x0ABE},
	{0x9050, 0x849A, 0x18F2, 0x72E0, 0x1674, 0x27AC, 0x0BFB, 0x097D, 0x0ABE},
	{0x1674, 0x27AC, 0xA188, 0x849A, 0x18F2, 0x0BFB, 0x097D, 0x0ABE},
	{0xA188, 0x65E9, 0x8359, 0x9050, 0x849A, 0x18F2, 0x72E0, 0x1674, 0x27AC, 0x0BFB, 0x097D, 0x0ABE},
	{0x8359, 0x849A, 0x18F2, 0x1674, 0x27AC, 0x65E9, 0x0BFB, 0x097D, 0x0ABE},
	{0x849A, 0x8359, 0x097D, 0x0ABE, 0x72E0, 0x1674, 0x27AC},
	{0xA188, 0x9050, 0x1674, 0x27AC, 0x849A, 0x8359, 0x097D, 0x0ABE},
	{0x097D, 0x0ABE, 0x849A, 0xA188, 0x65E9, 0x72E0, 0x1674, 0x27AC},
	{0x65E9, 0x097D, 0x0ABE, 0x849A, 0x9050, 0x1674, 0x27AC},
	{0x18F2, 0x9050, 0x8359, 0x097D, 0x0ABE, 0x72E0, 0x1674, 0x27AC},
	{0x18F2, 0x1674, 0x27AC, 0xA188, 0x8359, 0x097D, 0x0ABE},
	{0x9050, 0xA188, 0x65E9, 0x097D, 0x0ABE, 0x18F2, 0x72E0, 0x1674, 0x27AC},
	{0x18F2, 0x1674, 0x27AC, 0x65E9, 0x097D, 0x0ABE},
	{0x65E9, 0x27AC, 0x0ABE, 0x0BFB},
	{0x65E9, 0x27AC, 0x0ABE, 0x0BFB, 0xA188, 0x9050, 0x72E0},
	{0x8359, 0xA188, 0x27AC, 0x0ABE, 0x0BFB},
	{0x9050, 0x8359, 0x0BFB, 0x0ABE, 0x27AC, 0x72E0},
	{0x65E9, 0x27AC, 0x0ABE, 0x0BFB, 0x9050, 0x849A, 0x18F2},
	{0xA188, 0x849A, 0x18F2, 0x72E0, 0x0BFB, 0x65E9, 0x27AC, 0x0ABE},
	{0x8359, 0xA188, 0x27AC, 0x0ABE, 0x0BFB, 0x9050, 0x849A, 0x18F2},
	{0x8359, 0x849A, 0x18F2, 0x72E0, 0x27AC, 0x0ABE, 0x0BFB},
	{0x65E9, 0x27AC, 0x0ABE, 0x849A, 0x8359},
	{0x65E9, 0x27AC, 0x0ABE, 0x849A, 0x8359, 0xA188, 0x9050, 0x72E0},
	{0xA188, 0x27AC, 0x0ABE, 0x849A},
	{0x72E0, 0x27AC, 0x0ABE, 0x849A, 0x9050},
	{0x9050, 0x8359, 0x65E9, 0x27AC, 0x0ABE, 0x18F2},
	{0x8359, 0x65E9, 0x27AC, 0x0ABE, 0x18F2, 0x72E0, 0xA188},
	{0x9050, 0xA188, 0x27AC, 0x0ABE, 0x18F2},
	{0x72E0, 0x27AC, 0x0ABE, 0x18F2},
	{0x0ABE, 0x0BFB, 0x65E9, 0x72E0, 0x1674},
	{0x9050, 0x1674, 0x0ABE, 0x0BFB, 0x65E9, 0xA188},
	{0x72E0, 0x1674, 0x0ABE, 0x0BFB, 0x8359, 0xA188},
	{0x0BFB, 0x8359, 0x9050, 0x1674, 0x0ABE},
	{0x0ABE, 0x0BFB, 0x65E9, 0x72E0, 0x1674, 0x9050, 0x849A, 0x18F2},
	{0x1674, 0x0ABE, 0x0BFB, 0x65E9, 0xA188, 0x849A, 0x18F2},
	{0x0ABE, 0x0BFB, 0x8359, 0xA188, 0x72E0, 0x1674, 0x9050, 0x849A, 0x18F2},
	{0x8359, 0x849A, 0x18F2, 0x1674, 0x0ABE, 0x0BFB},
	{0x72E0, 0x65E9, 0x8359, 0x849A, 0x0ABE, 0x1674},
	{0x65E9, 0xA188, 0x9050, 0x1674, 0x0ABE, 0x849A, 0x8359},
	{0x1674, 0x0ABE, 0x849A, 0xA188, 0x72E0},
	{0x9050, 0x1674, 0x0ABE, 0x849A},
	{0x0ABE, 0x18F2, 0x9050, 0x8359, 0x65E9, 0x72E0, 0x1674},
	{0xA188, 0x8359, 0x65E9, 0x18F2, 0x1674, 0x0ABE},
	{0xA188, 0x72E0, 0x1674, 0x0ABE, 0x18F2, 0x9050},
	{0x18F2, 0x1674, 0x0ABE},
	{0x18F2, 0x0BFB, 0x097D, 0x1674},
	{0x0BFB, 0x097D, 0x1674, 0x18F2, 0xA188, 0x9050, 0x72E0},
	{0x0BFB, 0x097D, 0x1674, 0x18F2, 0xA188, 0x65E9, 0x8359},
	{0x8359, 0x9050, 0x72E0, 0x65E9, 0x18F2, 0x0BFB, 0x097D, 0x1674},
	{0x0BFB, 0x097D, 0x1674, 0x9050, 0x849A},
	{0xA188, 0x849A, 0x0BFB, 0x097D, 0x1674, 0x72E0},
	{0x0BFB, 0x097D, 0x1674, 0x9050, 0x849A, 0xA188, 0x65E9, 0x8359},
	{0x849A, 0x0BFB, 0x097D, 0x1674, 0x72E0, 0x65E9, 0x8359},
	{0x849A, 0x8359, 0x097D, 0x1674, 0x18F2},
	{0x849A, 0x8359, 0x097D, 0x1674, 0x18F2, 0xA188, 0x9050, 0x72E0},
	{0x1674, 0x097D, 0x65E9, 0xA188, 0x849A, 0x18F2},
	{0x849A, 0x9050, 0x72E0, 0x65E9, 0x097D, 0x1674, 0x18F2},
	{0x8359, 0x097D, 0x1674, 0x9050},
	{0xA188, 0x8359, 0x097D, 0x1674, 0x72E0},
	{0x65E9, 0x097D, 0x1674, 0x9050, 0xA188},
	{0x65E9, 0x097D, 0x1674, 0x72E0},
	{0x27AC, 0x72E0, 0x18F2, 0x0BFB, 0x097D},
	{0xA188, 0x27AC, 0x097D, 0x0BFB, 0x18F2, 0x9050},
	{0x27AC, 0x72E0, 0x18F2, 0x0BFB, 0x097D, 0xA188, 0x65E9, 0x8359},
	{0x27AC, 0x65E9, 0x8359, 0x9050, 0x18F2, 0x0BFB, 0x097D},
	{0x849A, 0x0BFB, 0x097D, 0x27AC, 0x72E0, 0x9050},
	{0x097D, 0x27AC, 0xA188, 0x849A, 0x0BFB},
	{0x27AC, 0x72E0, 0x9050, 0x849A, 0x0BFB, 0x097D, 0x8359, 0xA188, 0x65E9},
	{0x849A, 0x0BFB, 0x097D, 0x27AC, 0x65E9, 0x8359},
	{0x8359, 0x097D, 0x27AC, 0x72E0, 0x18F2, 0x849A},
	{0x18F2, 0x849A, 0x8359, 0x097D, 0x27AC, 0xA188, 0x9050},
	{0x097D, 0x27AC, 0x72E0, 0x18F2, 0x849A, 0xA188, 0x65E9},
	{0x9050, 0x18F2, 0x849A, 0x65E9, 0x097D, 0x27AC},
	{0x72E0, 0x9050, 0x8359, 0x097D, 0x27AC},
	{0x8359, 0x097D, 0x27AC, 0xA188},
	{0x9050, 0xA188, 0x65E9, 0x097D, 0x27AC, 0x72E0},
	{0x65E9, 0x097D, 0x27AC},
	{0x1674, 0x18F2, 0x0BFB, 0x65E9, 0x27AC},
	{0x1674, 0x18F2, 0x0BFB, 0x65E9, 0x27AC, 0xA188, 0x9050, 0x72E0},
	{0xA188, 0x27AC, 0x1674, 0x18F2, 0x0BFB, 0x8359},
	{0x27AC, 0x1674, 0x18F2, 0x0BFB, 0x8359, 0x9050, 0x72E0},
	{0x9050, 0x1674, 0x27AC, 0x65E9, 0x0BFB, 0x849A},
	{0x1674, 0x72E0, 0xA188, 0x849A, 0x0BFB, 0x65E9, 0x27AC},
	{0x0BFB, 0x8359, 0xA188, 0x27AC, 0x1674, 0x9050, 0x849A},
	{0x849A, 0x0BFB, 0x8359, 0x72E0, 0x27AC, 0x1674},
	{0x8359, 0x65E9, 0x27AC, 0x1674, 0x18F2, 0x849A},
	{0x1674, 0x18F2, 0x849A, 0x8359, 0x65E9, 0x27AC, 0xA188, 0x9050, 0x72E0},
	{0x18F2, 0x849A, 0xA188, 0x27AC, 0x1674},
	{0x849A, 0x9050, 0x72E0, 0x27AC, 0x1674, 0x18F2},
	{0x27AC, 0x1674, 0x9050, 0x8359, 0x65E9},
	{0x8359, 0x65E9, 0x27AC, 0x1674, 0x72E0, 0xA188},
	{0xA188, 0x27AC, 0x1674, 0x9050},
	{0x72E0, 0x27AC, 0x1674},
	{0x72E0, 0x18F2, 0x0BFB, 0x65E9},
	{0x9050, 0x18F2, 0x0BFB, 0x65E9, 0xA188},
	{0xA188, 0x72E0, 0x18F2, 0x0BFB, 0x8359},
	{0x9050, 0x18F2, 0x0BFB, 0x8359},
	{0x849A, 0x0BFB, 0x65E9, 0x72E0, 0x9050},
	{0xA188, 0x849A, 0x0BFB, 0x65E9},
	{0x72E0, 0x9050, 0x849A, 0x0BFB, 0x8359, 0xA188},
	{0x8359, 0x849A, 0x0BFB},
	{0x8359, 0x65E9, 0x72E0, 0x18F2, 0x849A},
	{0x18F2, 0x849A, 0x8359, 0x65E9, 0xA188, 0x9050},
	{0x72E0, 0x18F2, 0x849A, 0xA188},
	{0x9050, 0x18F2, 0x849A},
	{0x9050, 0x8359, 0x65E9, 0x72E0},
	{0xA188, 0x8359, 0x65E9},
	{0xA188, 0x72E0, 0x9050},
	{}
};
