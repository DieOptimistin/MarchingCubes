/*****************************************************************************\
*                             Isosurface
\*****************************************************************************/
/*! @class Isosurface
*
*  @brief
*  creates an isosurface object
*
*  @author Anthea Sander
*
*/

//---------------------------------------------------------------------------
//  Includes
//---------------------------------------------------------------------------

#include "Isosurface.h"
#include <bitset>

/********************** constructors & destructors *************************/

/* Initiates and starts the mesh generation process
*
*  @param skeleton (in) all skeleton nodes that this isosurface should represent, if it's empty no exception will be thrown!
*  @param cubeSize (in) the size of a singe cube, should be smaller than the smallest skeleton node. A small size has a huge impact on performance.
*  @param blend (in) the blending function that is used to calculate the scalar field
*  @param targetValue (in) threshold what scalarvalues will be interpreted as inside the object or as outside the object
*
* The mesh is generated using the marching cubes algorithm based on the implementation of Paul Bourke. http://paulbourke.net/geometry/polygonise/
* For a better performance this algorithm contains some differences. For example the scalarfield is calculated beforehand. This implementation
* don't uses any threading and works best for small objects. Fell free to start the initiate method in it's own thread.
* The result will be a vertex, normal and triangle list. You can access them directly.
*
*/
void Isosurface::calculate(
	const std::vector<SkeletonNode*> &skeleton,
	const float &cubeSize,
	BlendingFunction blend,
	const float &targetValue)
{
	mSkeleton = skeleton;
	mCubeSize = cubeSize;
	mBlendingFunction = blend;
	mTargetValue = targetValue;

	//clear all old data
	mVertices.clear();
	mNormals.clear();
	mTriangles.clear();
	mVertexHash.clear();
	mExtendsFrom = FLT_MAX;
	mExtendsTo = -FLT_MAX;

	//make sure that the given parameters make sense
	assert(mCubeSize > 0);
	assert(mBlendingFunction >= 0 && mBlendingFunction <= NONE);

	//only generate an object if it contains any nodes
	if (mSkeleton.size() > 0)
	{
		//calculate the total bounding box
		for (int i = 0; i < mSkeleton.size(); i++)
		{
			// find the object extends
			if (mSkeleton[i]->extendsTo.x > mExtendsTo.x) mExtendsTo.x = mSkeleton[i]->extendsTo.x;
			if (mSkeleton[i]->extendsTo.y > mExtendsTo.y) mExtendsTo.y = mSkeleton[i]->extendsTo.y;
			if (mSkeleton[i]->extendsTo.z > mExtendsTo.z) mExtendsTo.z = mSkeleton[i]->extendsTo.z;

			if (mSkeleton[i]->extendsFrom.x < mExtendsFrom.x) mExtendsFrom.x = mSkeleton[i]->extendsFrom.x;
			if (mSkeleton[i]->extendsFrom.y < mExtendsFrom.y) mExtendsFrom.y = mSkeleton[i]->extendsFrom.y;
			if (mSkeleton[i]->extendsFrom.z < mExtendsFrom.z) mExtendsFrom.z = mSkeleton[i]->extendsFrom.z;
		}


		generateObject();
	}
};

void Isosurface::update(const std::vector<SkeletonNode*> &skeleton)
{
	mSkeleton = skeleton;

	mVertices.clear();
	mNormals.clear();
	generateObject();
};

/* conversion of the index of a three dimensional field to an one dimensional field,
*  takes the coordinates and dimensions of the three dimensional field
*/
static int getMapIndex(
	const int &x,
	const int &y,
	const int &z,
	const int &width,
	const int &height,
	const int &depth)
{
	return z*width*height + y*width + x;
}


/********************** marching cubes algorithm  *************************/
/* the actual mesh generation process
*/
void Isosurface::generateObject()
{
	const Ogre::Vector3 &boundingBox(mExtendsTo - mExtendsFrom);

	//make sure the object size contains at least one voxel
	if (boundingBox.x > mCubeSize
		&& boundingBox.y > mCubeSize
		&& boundingBox.z > mCubeSize)
	{
		//amount of cubes in every direction, make it +1 to ensure that all voxels are included
		int xAmount = boundingBox.x / mCubeSize + 1;
		int yAmount = boundingBox.y / mCubeSize + 1;
		int zAmount = boundingBox.z / mCubeSize + 1;

		//scalarfield contains scalar value for every cube corner, thus +1 dimension
		std::vector<float> scalarField((xAmount + 1)*(yAmount + 1)*(zAmount + 1));

		//fill the array with -1, so that we can check later if all positions contain a valid value
		for (int i = 0; i < scalarField.size(); i++)
			scalarField[i] = -1;

		//calculate the scalar value for the complete voxelgrid
		fillScalarField(scalarField, xAmount, yAmount, zAmount);

		//every vertex has an unique index, that is used for triangle specification
		int iVertexIndex = 0;

		//march cubes
		for (int z = 0; z < zAmount; z++)
		{
			for (int y = 0; y < yAmount; y++)
			{
				for (int x = 0; x < xAmount; x++)
				{
					//create a local copy of the current cell
					Cell cell;
					for (int i = 0; i < 8; i++)
					{
						int index = getMapIndex(
							x + cubeOffsets[i][0],
							y + cubeOffsets[i][1],
							z + cubeOffsets[i][2],
							xAmount + 1, yAmount + 1, zAmount + 1);

						//the absolute position of all cubes corners
						cell.p[i].x = mExtendsFrom.x + mCubeSize * (float)(x + cubeOffsets[i][0]);
						cell.p[i].y = mExtendsFrom.y + mCubeSize * (float)(y + cubeOffsets[i][1]);
						cell.p[i].z = mExtendsFrom.z + mCubeSize * (float)(z + cubeOffsets[i][2]);

						cell.val[i] = scalarField.at(index);

						//generate a look up index for this specific edge intersection combination
						if (cell.val[i] <= mTargetValue)
							cell.iFlagIndex |= 1 << i;
					}

					int nTriangles = 0;
					Triangle triangles[5];
					Triangulate(cell, triangles, &nTriangles);

					// fill the "vertex buffer" and the "index buffer"
					for (int iTriangle = 0; iTriangle < nTriangles; iTriangle++)
					{
						IndexedTriangle indexedTriangle;

						for (int iCorner = 0; iCorner < 3; iCorner++)
						{
							VertexKey v = { triangles[iTriangle].p[iCorner] };

							int currentVertexIndex = -1;

							IndexBuffer::iterator iter(mVertexHash.find(v));
							if (iter == mVertexHash.end()) {    // not found

								//insert uniquely
								mVertexHash.insert(iter, std::make_pair(v, iVertexIndex));
								currentVertexIndex = iVertexIndex;
								mVertices.push_back(triangles[iTriangle].p[iCorner]);

								//calculate the normal for that vertex
								Ogre::Vector3 normal;
								getNormal(triangles[iTriangle].p[iCorner], normal);
								mNormals.push_back(normal);

								iVertexIndex++;
							}
							else { // the vertex was already generated, reuse it
								currentVertexIndex = iter->second;
							}
							indexedTriangle.i[iCorner] = currentVertexIndex;

						}

						mTriangles.push_back(indexedTriangle);
					}
				}
			}
		}
	}

}

/* finds all triangles in the given cube/cell
* @param cell (in) local cube, must contain all absolute corner positions, their scalar value and iFlagIndex that determines what corner is inside or outside
* @param triangles (out) the list of calculated triangles with absolute positions, maximal 5
* @param nTriangles (out) the amount of triangles, maximal 5
*/
void Isosurface::Triangulate(Cell &cell, Triangle *triangles, int *nTriangles)
{
	//look up 
	cell.iEdgeFlags = aiCubeEdgeFlags[cell.iFlagIndex];

	//If the cube is entirely inside or outside of the surface, then there will be no intersections
	if (cell.iEdgeFlags == 0)
	{
		return;
	}

	Ogre::Vector3 asEdgeVertex[12];

	//Find the point of intersection of the surface with each edge
	//Then find the normal to the surface at those points
	for (int iEdge = 0; iEdge < 12; iEdge++)
	{
		//if there is an intersection on this edge
		if (cell.iEdgeFlags & (1 << iEdge))
		{
			const int* edge = a2iEdgeConnection[iEdge];
			const int* edgeDir = a2fEdgeDirection[iEdge];

			int p1 = edge[0];
			int p2 = edge[1];

			//find the approx intersection point by linear interpolation between the two edges and the density value
			//asEdgeVertex[iEdge] = cell.p[p1] + (cell.p[p2] - cell.p[p1]) * cell.val[p1] / (cell.val[p2] + cell.val[p1]);
			float length = cell.val[p1] / (cell.val[p2] + cell.val[p1]);
			asEdgeVertex[iEdge] = cell.p[p1] + length  * (cell.p[p2] - cell.p[p1]);
		}
	}

	//Draw the triangles that were found. There can be up to five per cube
	for (int iTriangle = 0; iTriangle < 5; iTriangle++)
	{
		//no triangles in this cube
		if (a2iTriangleConnectionTable[cell.iFlagIndex][3 * iTriangle] < 0)
			break;

		(*nTriangles)++;
		for (int iCorner = 0; iCorner < 3; iCorner++)
		{
			int iVertex = a2iTriangleConnectionTable[cell.iFlagIndex][3 * iTriangle + iCorner];

			triangles[iTriangle].p[iCorner] = asEdgeVertex[iVertex];
		}
	}

}

/* calculates a scalarfield for an given cube amount
* @param scalarField (out) the final scalar field that contains all scalar values
* @param xAmount, yAmount, zAmount (in) the dimension to all sides
*
* the scalarField will be calculated by calling IsoValue(coord) for every cubes corner.
* the final vector is a threedimensional array converted into a onedimensional array.
* @see getMapIndex(..) for the conversion of the index
*/
void Isosurface::fillScalarField(std::vector<float> &scalarField, const int &xAmount, const int &yAmount, const int &zAmount)
{
	//calculates for every voxel corner the scalar value
	for (int z = 0; z < zAmount + 1; z++)
	{
		for (int y = 0; y < yAmount + 1; y++)
		{
			for (int x = 0; x < xAmount + 1; x++)
			{
				const Ogre::Vector3 coords(
					mExtendsFrom.x + (float)(x * mCubeSize),
					mExtendsFrom.y + (float)(y * mCubeSize),
					mExtendsFrom.z + (float)(z * mCubeSize)
					);

				int index = getMapIndex(x, y, z, xAmount + 1, yAmount + 1, zAmount + 1);

				assert(scalarField[index] == -1); //make sure that every position is visited once
				scalarField[index] = IsoValue(coords);
			}
		}
	}
}

/*vGetNormal() finds the gradient of the scalar field at a point
*
*/
void Isosurface::getNormal(const Ogre::Vector3 &vertex, Ogre::Vector3 &normal) const
{
	float iso_x1 = IsoValue(Ogre::Vector3(vertex.x - mCubeSize, vertex.y, vertex.z));
	float iso_x2 = IsoValue(Ogre::Vector3(vertex.x + mCubeSize, vertex.y, vertex.z));

	float iso_y1 = IsoValue(Ogre::Vector3(vertex.x, vertex.y - mCubeSize, vertex.z));
	float iso_y2 = IsoValue(Ogre::Vector3(vertex.x, vertex.y + mCubeSize, vertex.z));

	float iso_z1 = IsoValue(Ogre::Vector3(vertex.x, vertex.z, vertex.z - mCubeSize));
	float iso_z2 = IsoValue(Ogre::Vector3(vertex.x, vertex.y, vertex.z + mCubeSize));

	normal.x = iso_x1 - iso_x2;
	normal.y = iso_y1 - iso_y2;
	normal.z = iso_z1 - iso_z2;

	normal.normalise();
}

/* calculates the scalar value of a given absolute point
* therefore the distance to all skeleton nodes is taken into account.
*
* @param pos the absolute position for that the isovalue will be calculates
* @return the scalar value [0 .. Infinity]
*/
float Isosurface::IsoValue(const Ogre::Vector3 &pos) const
{
	float potential = 0.0f;


	for (int i = 0; i < mSkeleton.size(); i++)
	{
		SkeletonNode *node = mSkeleton.at(i);

		const float &d(node->radius * 10);
		const float &r(node->getDistanceTo(pos) / (node->radius));

		if (r <= 1)
		{
			potential += (Ogre::Math::Pow(r, 4) - 2 * r*r + 1) / (1 + d* r * r);
		}

		break;
	}

	return potential;

}

