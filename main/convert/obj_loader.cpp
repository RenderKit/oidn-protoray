// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "sys/logging.h"
#include "obj_loader.h"
#include <fstream>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>

namespace prt {

ObjLoader::ObjLoader()
{
  bufferSize = 256 * 1024;
  buffer = new char[bufferSize];
  currentMaterialId = -1;
  degenerateTriangleCount = 0;
}

ObjLoader::~ObjLoader()
{
  delete[] buffer;
}

bool ObjLoader::load(const std::string& filename, TriangleSoup& soup, bool compressed)
{
  if (compressed)
    Log() << "Loading compressed OBJ file: " << filename;
  else
    Log() << "Loading OBJ file: " << filename;

  if (!parseObj(filename, compressed))
  {
    cleanup();
    return false;
  }

  if (!makeSoup(soup))
  {
    cleanup();
    return false;
  }

  cleanup();
  return true;
}

bool ObjLoader::parseObj(const std::string& filename, bool compressed)
{
  std::ios_base::openmode openMode = std::ios_base::in;
  if (compressed)
    openMode |= std::ios_base::binary;
  std::ifstream file(filename, openMode);
  if (!file.is_open())
  {
    LogError() << "Could not open OBJ file: " << filename;
    return false;
  }

  boost::iostreams::filtering_istream input;
  if (compressed)
    input.push(boost::iostreams::gzip_decompressor());
  input.push(file);

  materialNameList.pushBack(""); // 0 is reserved for background
  currentMaterialId = -1;

  degenerateTriangleCount = 0;

  std::string line;
  while (std::getline(input, line))
  {
    std::istringstream lineStream(line);
    std::string head;
    lineStream >> head >> std::ws;

    bool ok = true;
    std::getline(lineStream, line);
    strncpy(buffer, line.c_str(), bufferSize);

    if (head == "v")
    {
      ok = parsePosition();
    }
    else if (head == "vn")
    {
      ok = parseNormal();
    }
    else if (head == "vt")
    {
      ok = parseTexcoord();
    }
    else if (head == "f")
    {
      ok = parseFace();
    }
    else if (head == "usemtl")
    {
      ok = parseUseMtl();
    }

    if (!ok)
      return false;
  }

  if (degenerateTriangleCount > 0)
    LogWarn() << "Removed " << toString(degenerateTriangleCount) << " degenerate triangles";

  //Log() << "v: " << positionList_.getSize();
  //Log() << "vn: " << normalList_.getSize();
  //Log() << "vt: " << texcoordList_.getSize();

  return true;
}

bool ObjLoader::parsePosition()
{
  Vec3f position;
  if (sscanf(buffer, "%f %f %f", &position.x, &position.y, &position.z) != 3)
  {
    LogError() << "Invalid vertex position";
    return false;
  }
  positionList.pushBack(position);
  return true;
}

bool ObjLoader::parseNormal()
{
  Vec3f normal;
  if (sscanf(buffer, "%f %f %f", &normal.x, &normal.y, &normal.z) != 3)
  {
    LogError() << "Invalid vertex normal";
    return false;
  }
  normalList.pushBack(normal);
  return true;
}

bool ObjLoader::parseTexcoord()
{
  Vec2f texcoord;
  if (sscanf(buffer, "%f %f", &texcoord.x, &texcoord.y) != 2)
  {
    LogError() << "Invalid vertex texture coordinates";
    return false;
  }
  texcoordList.pushBack(texcoord);
  return true;
}

bool ObjLoader::parseFace()
{
  // Parse the vertices
  faceVertexList.clear();

  const char* delimiters = " \t\n\r";
  char* token = strtok(buffer, delimiters);
  bool ok = true;

  while (token != 0)
  {
    ObjVertex v;

    if (sscanf(token, "%d/%d/%d", &v.p, &v.t, &v.n) == 3)
    {
      // Position, texcoord, normal
    }
    else if (sscanf(token, "%d//%d", &v.p, &v.n) == 2)
    {
      // Position, normal
      v.t = 0;
    }
    else if (sscanf(token, "%d/%d", &v.p, &v.t) == 2)
    {
      // Position, texcoord
      v.n = 0;
    }
    else if (sscanf(token, "%d", &v.p) == 1)
    {
      // Position
      v.t = 0;
      v.n = 0;
    }
    else
    {
      ok = false;
    }

    v.p = convertIndex(v.p, positionList.getSize());
    v.t = convertIndex(v.t, texcoordList.getSize());
    v.n = convertIndex(v.n, normalList.getSize());

    faceVertexList.pushBack(v);

    // Next token
    token = strtok(0, delimiters);
  }

  if (!ok || faceVertexList.getSize() < 3)
  {
    LogError() << "Invalid face";
    return false;
  }

  // Split the face into triangles
  if (currentMaterialId < 0)
    currentMaterialId = materialNameList.pushBack("Default");

  ObjTriangle face;
  face.matId = currentMaterialId;

  for (int i = 0; i < faceVertexList.getSize() - 2; ++i)
  {
    face.v[0] = faceVertexList[0];
    face.v[1] = faceVertexList[i + 1];
    face.v[2] = faceVertexList[i + 2];

    if (face.v[0].p == face.v[1].p || face.v[0].p == face.v[2].p || face.v[1].p == face.v[2].p)
      ++degenerateTriangleCount;
    else
      triangleList.pushBack(face);
  }

  return true;
}

bool ObjLoader::parseUseMtl()
{
  char nameC[256];
  if (sscanf(buffer, "%255s", nameC) != 1)
  {
    LogError() << "Invalid material";
    return false;
  }
  std::string name = nameC;

  currentMaterialId = materialNameList.indexOf(name);
  if (currentMaterialId < 0)
    currentMaterialId = materialNameList.pushBack(name);

  return true;
}

int ObjLoader::convertIndex(int i, int size)
{
  if (i >= 0)
    return i;
  return i + size + 1;
}

bool ObjLoader::makeSoup(TriangleSoup& soup)
{
  bool ok = true;
  soup.triangles.alloc(triangleList.getSize());

  for (int iTri = 0; iTri < triangleList.getSize(); ++iTri)
  {
    bool okNormal = true;

    for (int iVert = 0; iVert < 3; ++iVert)
    {
      // Position
      int positionId = triangleList[iTri].v[iVert].p;
      if (positionId > positionList.getSize())
      {
        ok = false;
        goto done;
      }
      soup.triangles[iTri].v[iVert].pos = positionList[positionId - 1];

      // Normal
      int normalId = triangleList[iTri].v[iVert].n;
      if (normalId > 0 && normalId <= normalList.getSize())
      {
        soup.triangles[iTri].v[iVert].normal = normalList[normalId - 1];
      }
      else if (normalId == 0)
      {
        okNormal = false;
      }
      else
      {
        ok = false;
        goto done;
      }

      // Texcoord
      int texcoordId = triangleList[iTri].v[iVert].t;
      if (texcoordId > 0 && texcoordId <= texcoordList.getSize())
      {
        soup.triangles[iTri].v[iVert].tex = texcoordList[texcoordId - 1];
      }
      else if (texcoordId == 0)
      {
        soup.triangles[iTri].v[iVert].tex = zero;
      }
      else
      {
        ok = false;
        goto done;
      }
    }

    Vec3f Ng = soup.triangles[iTri].getNormal();

    // Fix bad shading normals
    if (okNormal)
    {
      for (int iVert = 0; iVert < 3; ++iVert)
        if (abs(dot(soup.triangles[iTri].v[iVert].normal, Ng)) < 1e-5f)
          okNormal = false;
    }

    if (!okNormal)
    {
      for (int iVert = 0; iVert < 3; ++iVert)
        soup.triangles[iTri].v[iVert].normal = Ng;
    }

    // Material
    soup.triangles[iTri].matId = triangleList[iTri].matId;
  }

done:
  if (!ok)
  {
    LogError() << "Invalid vertex reference";
    soup.triangles.free();
    return false;
  }

  // Copy the material names
  soup.materialNames = materialNameList;

  return true;
}

void ObjLoader::cleanup()
{
  positionList.free();
  texcoordList.free();
  normalList.free();
  triangleList.free();
  materialNameList.free();
  faceVertexList.free();
}

} // namespace prt
