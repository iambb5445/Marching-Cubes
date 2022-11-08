# Marching Cubes

This is an implementation of the marching cubes algorithm. Marching cubes is an algorithm to extract a mesh from a 3D scalar field. By giving a 3D scalar field (called volumetric data in this implementation) as an input, it will generate the mesh data for that shape in a custom format.

This was implemented as a project for the Game Graphics Course at UCSC, Winter 2022, instructed by Eric Lengyl. I originally implemented this in the provided Game Engine, but I extracted this as an standalone implementation of marching cubes to share on GitHub.

# How to Use

As mentioned above, the marching cubes algorithm gets a 3D volumetric data as input and generates a mesh. For the input, you can either write your data in the code, as a .txt file, or by using one of the existing examples in `Main.cpp`. Please refer to the `main` function in the `Main.cpp` file for more examples.

The input file format is defined as follows:
```
<xdim> <ydim> <zdim>
<data_0> <data_1> ... <data_xdim*ydim*zdim> // integer values in range [-1, 1], space separated
```

The output will be generated as a `MarchedGeometry` class, which will be written in an output file in a custom format. The output format is as follows:
```
<vertex_count>
<pos_x 0> <pos_y 0> <pox_z 0>
<pos_x 1> <pos_y 1> <pox_z 1>
...
<pos_x vertex_count-1> <pos_y vertex_count-1> <pox_z vertex_count-1>
<triangle_count>
<triangle 0> <triangle 0> <triangle 0> // 3 vertex indices for this triangle, each in range [0, vertex_count)
<triangle 1> <triangle 1> <triangle 1>
...
<triangle triangle_count-1> <triangle triangle_count-1> <triangle triangle_count-1>
```

# Implementation Details

This implementation of the Marching cubes algorithm follows the description of the Foundations of Game Engine Development book by Eric Lengyl.

The idea behind the marching cubes algorithm is as follows:

Assuming we have an scalar field, the goal of this algorithm is to extract an isosurface from it. In order to do that, we can consider every cube with scalar values on its corners and find the triangles for it. These triangles can be found by using a lookup table.

Because a cube has 8 corners, there are 2^8 different distinct cases for the triangles, but because many of them are actually equivalent to each other, we can reduce the size of this lookup table to contain the triangles for the 18 different equivalence classes (considering prefered polarity, refer to the book for more information).

After looking up the triangles for a single cube, we should consider the exact number on each of the corners to find out where the triangle vertices should be on the edge. For example, an edge of the cube containing -1 and 1 on the ends should have the triangle vertex in the middle. But if the corner values are -1 and 0.1, the vertex should be more toward the 0.1 corner.

Finally, the algorithm should consider where two vertices are the same to make sure the vertices are being reused in the final mesh. This is required both for preventing artifacts and to reduce the size of the mesh data.

## Code Details

There are a few helper classes implemented in this code such as `Vector3D` (containing 3 floating point values), `Vertex` (containing one single position in form of a `Vector3D`) and `Triangle` (containing 3 indexes referring to the 3 vertices that create this triangle).

The `VolumentricData` class is used to store any 3D data and read/write it from/to file.

The main implementation of the Marching Cubes algorithm is in the `MarchedGeometry` class. When a `MarchedGeometry` class is created, an scale and a volumetric data is given to it. Then, in its constructor, the private `MarchCubes` function will be called.

When marching each cube:
1. The case index will be calculated in the `getCaseIndex` function. The case index is one the 256 different possible cases for the cube. The case index is an 8 bit value, each bit showing the sign of the value on one of the corners of the cube. Note that the sign is the only imporatnt factor for the case index, the exact value is for moving the triangle vertex on the cube edge.
2. The class index will be calcualted by looking up the case index (256 different cases) in the `caseIndexToClassIndex` table. This contains one of the 18 values of the different 18 equivalence classes for each of the 256 cases. This is more efficient that storing the triangles for all the 256 cases.
3. The class index will be looked up in the `ClassGeometry` table. This contains an 8 bit number (4 bit for the triangle count, 4 bit for the vertex count, implemeted as one 8-bit number with C++'s `union` syntax), and a list of triangle_count*3 of the triangle vertex indices for that equivalence class.
4. The case index will be looked up in the `onEdgeVertexCode` table. Note that this table has 256 values instead of 18, because the actual edges of the cube which contain triangle vertices differ based on case for each class. This table contains the index of the lower corner (3 bits), index of the higher corner (3 bits), reduced edge index (2 bits), edge index (4 bit) and edge delta (4 bits), as a 16-bit value. Their use will be explained in the steps 5 and 6.
5. The code will be used to find the exact values of the lower and higher corner of that cube and interpolate the vertex on the edge away from the stronger value. We call this coefficient `t`.
6. There are two possibilities for `t`:
   1. t is 0(min value) or 256(max value). In this case, the triangle vertex is on the one of the corners. This is important to consider separately from 6.2. to prevent using duplicate vertices for the same point and prevent artifacts.
   2. t is not 0 or 256. In this case, the triangle vertex is on the edge between two vertices. The exact position can be calculated by considering the value of t.

In both cases, the value of edge delta and edge index will be combined to find out if the vertex should already exist or not. If so, we can find the value from one of the neighboring cubes. Otherwise, a new vertex will be added.

7. Finally, we add all the triangles, and check no to add a triangle with the area of zero (all vertices on the same point, which is possible in some cases).

Note that the data we keep from vertices and edges to find an existing vertex is kept but only for the last two planes of cubes. Any cube with an existing edge has that edge in one the neighbors whose edges was previously calculated, so this neighbor should be either on the same plane of cubes (same z value) or one above (z-1).

# Future Work

- More output formats (.obj, .fbx, .blend, etc.)
- Rendering the output
- Creating an Unreal Engine or Unity plugin from this
- Better exception handling (using custom exception, catching exception and printing appropriate error messages)
- Setting normals, tangent and textcoord in the generated mesh