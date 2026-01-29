// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "triangle.h"

namespace prt {

void splitPrim(const Triangle& tri, const Box3f& box, int axis, float pos, Box3f& leftBox, Box3f& rightBox)
{
  int k = axis;

  // Sort the vertices along the split dimension (min, mid, max)
  int vMin = tri.v[1][k] <= tri.v[0][k];
  int vMax = 1 - vMin;

  if (tri.v[2][k] <= tri.v[vMin][k])
    vMin = 2;

  if (tri.v[2][k] > tri.v[vMax][k])
    vMax = 2;

  int vMid = 3 - vMin - vMax;

  // Determine which side of a splitting plane contains 2 vertices by checking position of vertex mid
  bool isOneOnLeft = pos <= tri.v[vMid][k];

  // Reindex the vertices so A is the common vertex of 2 edges intersected by the splitting plane and B, C are 2 remaining vertices
  int a = isOneOnLeft ? vMin : vMax;
  int b = vMid;
  int c = 3 - a - b;

  // Compute the left and right AABBs
  float d = pos - tri.v[a][k];
  float tAB = clamp(d / (tri.v[b][k] - tri.v[a][k]), 0.0f, 1.0f);
  float tAC = clamp(d / (tri.v[c][k] - tri.v[a][k]), 0.0f, 1.0f);

  int u = (k == 0);
  int v = 3 - k - u;

  float cutABU = lerp(tri.v[a][u], tri.v[b][u], tAB);
  float cutABV = lerp(tri.v[a][v], tri.v[b][v], tAB);

  float cutACU = lerp(tri.v[a][u], tri.v[c][u], tAC);
  float cutACV = lerp(tri.v[a][v], tri.v[c][v], tAC);

  float cutMinU = min(cutABU, cutACU);
  float cutMinV = min(cutABV, cutACV);

  float cutMaxU = max(cutABU, cutACU);
  float cutMaxV = max(cutABV, cutACV);

  if (isOneOnLeft)
  {
    // Left: A, intersection points
    leftBox.low[u] = min(cutMinU, tri.v[a][u]);
    leftBox.low[v] = min(cutMinV, tri.v[a][v]);

    leftBox.high[u] = max(cutMaxU, tri.v[a][u]);
    leftBox.high[v] = max(cutMaxV, tri.v[a][v]);

    // Right: B, C, intersection points
    rightBox.low[u] = min(cutMinU, min(tri.v[b][u], tri.v[c][u]));
    rightBox.low[v] = min(cutMinV, min(tri.v[b][v], tri.v[c][v]));

    rightBox.high[u] = max(cutMaxU, max(tri.v[b][u], tri.v[c][u]));
    rightBox.high[v] = max(cutMaxV, max(tri.v[b][v], tri.v[c][v]));
  }
  else
  {
    // Left: B, C, intersection points
    leftBox.low[u] = min(cutMinU, min(tri.v[b][u], tri.v[c][u]));
    leftBox.low[v] = min(cutMinV, min(tri.v[b][v], tri.v[c][v]));

    leftBox.high[u] = max(cutMaxU, max(tri.v[b][u], tri.v[c][u]));
    leftBox.high[v] = max(cutMaxV, max(tri.v[b][v], tri.v[c][v]));

    // Right: A, intersection points
    rightBox.low[u] = min(cutMinU, tri.v[a][u]);
    rightBox.low[v] = min(cutMinV, tri.v[a][v]);

    rightBox.high[u] = max(cutMaxU, tri.v[a][u]);
    rightBox.high[v] = max(cutMaxV, tri.v[a][v]);
  }

  leftBox.low[k] = box.low[k];
  rightBox.high[k] = box.high[k];
  leftBox.high[k] = rightBox.low[k] = pos;

  // Intersect the left AABB with the original AABB
  leftBox.low[u] = max(leftBox.low[u], box.low[u]);
  leftBox.low[v] = max(leftBox.low[v], box.low[v]);

  leftBox.high[u] = min(leftBox.high[u], box.high[u]);
  leftBox.high[v] = min(leftBox.high[v], box.high[v]);

  // Intersect the right AABB with the original AABB
  rightBox.low[u] = max(rightBox.low[u], box.low[u]);
  rightBox.low[v] = max(rightBox.low[v], box.low[v]);

  rightBox.high[u] = min(rightBox.high[u], box.high[u]);
  rightBox.high[v] = min(rightBox.high[v], box.high[v]);
}

} // namespace prt
