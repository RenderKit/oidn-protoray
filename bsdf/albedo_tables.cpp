// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "sys/memory.h"
#include "albedo.h"
#include "albedo_tables.h"

namespace prt {

float* MicrofacetAlbedoTable::dir = nullptr;
float* MicrofacetAlbedoTable::avg = nullptr;
float* MicrofacetDielectricAlbedoTable::etaDir = nullptr;
float* MicrofacetDielectricAlbedoTable::etaAvg = nullptr;
float* MicrofacetDielectricAlbedoTable::rcpEtaDir = nullptr;
float* MicrofacetDielectricAlbedoTable::rcpEtaAvg = nullptr;
float* MicrofacetDielectricReflectionAlbedoTable::etaDir = nullptr;
float* MicrofacetDielectricReflectionAlbedoTable::etaAvg = nullptr;
float* MicrofacetDielectricReflectionECAlbedoTable::etaDir = nullptr;
float* MicrofacetDielectricReflectionECAlbedoTable::etaAvg = nullptr;
float* MicrofacetConductorArtisticECAlbedoTable::dir = nullptr;
float* MicrofacetSheenAlbedoTable::dir = nullptr;

float integrateAvg(const float* f, int size, int numSamples = 1024)
{
  // Trapezoidal rule
  const int n = numSamples+1;
  float sum = 0.f;
  for (int i = 0; i < n; ++i)
  {
    const float cosThetaO = (float)i / (n-1);
    const float x = cosThetaO * (size-1);
    sum += interp1DLinear(x, f, size) * cosThetaO * ((i == 0 || i == n-1) ? 0.5f : 1.f);
  }

  return min(2.f * (sum / (n-1)), 1.f);
}

void MicrofacetAlbedoTable::precompute()
{
  float* dirPtr = dir = (float*)alignedAlloc(size*size * sizeof(float));
  float* avgPtr = avg = (float*)alignedAlloc(size * sizeof(float));

  for (int j = 0; j < size; j++)
  {
    const float roughness = (float)j / (size-1);
    // compute the direction albedo for each cosThetaO
    for (int i = 0; i < size; ++i)
    {
      const float cosThetaO = (float)i / (size-1);
      dirPtr[i] = MicrofacetAlbedo::integrate(cosThetaO, roughness);
    }

    // compute the average albedo
    *avgPtr = integrateAvg(dirPtr, size);

    dirPtr += size;
    avgPtr++;
  }
}

template <class Albedo>
void precomputeDielectricAlbedoTable(int size,
                   float minEta, float maxEta,
                   float* dirValues,
                   float* avgValues)
{
  const int numSamples = 1024;

  float* dirPtr = dirValues;
  float* avgPtr = avgValues;

  for (int k = 0; k < size; k++)
  {
    const float roughness = (float)k / (size-1);
    for (int j = 0; j < size; j++)
    {
      const float eta = lerp(minEta, maxEta, (float)j / (size-1));
      // compute the direction albedo for each cosThetaO
      for (int i = 0; i < size; ++i)
      {
        const float cosThetaO = (float)i / (size-1);
        dirPtr[i] = Albedo::integrate(cosThetaO, eta, roughness, numSamples);
      }

      // compute the average albedo
      *avgPtr = integrateAvg(dirPtr, size);

      dirPtr += size;
      avgPtr++;
    }
  }
}

void MicrofacetDielectricAlbedoTable::precompute()
{
  const float minEta = rcp(maxIor);
  const float maxEta = rcp(minIor);
  etaDir = (float*)alignedAlloc(size*size*size * sizeof(float));
  etaAvg = (float*)alignedAlloc(size*size * sizeof(float));
  precomputeDielectricAlbedoTable<MicrofacetDielectricAlbedo>(size, minEta, maxEta, etaDir, etaAvg);

  const float minRcpEta = minIor;
  const float maxRcpEta = maxIor;
  rcpEtaDir = (float*)alignedAlloc(size*size*size * sizeof(float));
  rcpEtaAvg = (float*)alignedAlloc(size*size * sizeof(float));
  precomputeDielectricAlbedoTable<MicrofacetDielectricAlbedo>(size, minRcpEta, maxRcpEta, rcpEtaDir, rcpEtaAvg);
}

void MicrofacetDielectricReflectionAlbedoTable::precompute()
{
  const float minEta = rcp(maxIor);
  const float maxEta = rcp(minIor);
  etaDir = (float*)alignedAlloc(size*size*size * sizeof(float));
  etaAvg = (float*)alignedAlloc(size*size * sizeof(float));
  precomputeDielectricAlbedoTable<MicrofacetDielectricReflectionAlbedo>(size, minEta, maxEta, etaDir, etaAvg);
}

void MicrofacetDielectricReflectionECAlbedoTable::precompute()
{
  const float minEta = rcp(maxIor);
  const float maxEta = rcp(minIor);
  etaDir = (float*)alignedAlloc(size*size*size * sizeof(float));
  etaAvg = (float*)alignedAlloc(size*size * sizeof(float));
  precomputeDielectricAlbedoTable<MicrofacetDielectricReflectionECAlbedo>(size, minEta, maxEta, etaDir, etaAvg);
}

void MicrofacetConductorArtisticECAlbedoTable::precompute()
{
  float* dirPtr = dir = (float*)alignedAlloc(size*size*size*size * sizeof(float));

  for (int iRoughness = 0; iRoughness < size; iRoughness++)
  {
    const float roughness = (float)iRoughness / (size-1);
    for (int iG = 0; iG < size; iG++)
    {
      const float g = (float)iG / (size-1);

      for (int iR = 0; iR < size; iR++)
      {
        const float r = (float)iR / (size-1);

        // compute the direction albedo for each cosThetaO
        for (int i = 0; i < size; ++i)
        {
          const float cosThetaO = (float)i / (size-1);
          dirPtr[i] = MicrofacetConductorArtisticECAlbedo::integrate(cosThetaO, r, g, roughness, 256);
        }

        dirPtr += size;
      }
    }
  }
}

void MicrofacetSheenAlbedoTable::precompute()
{
  float* dirPtr = dir = (float*)alignedAlloc(size*size * sizeof(float));

  for (int j = 0; j < size; j++)
  {
    const float roughness = (float)j / (size-1);
    // compute the direction albedo for each cosThetaO
    for (int i = 0; i < size; ++i)
    {
      const float cosThetaO = (float)i / (size-1);
      dirPtr[i] = MicrofacetSheenAlbedo::integrate(cosThetaO, roughness);
    }

    dirPtr += size;
  }
}

void precomputeAlbedoTables()
{
  if (MicrofacetAlbedoTable::dir)
    return;

  MicrofacetAlbedoTable::precompute();
  MicrofacetDielectricAlbedoTable::precompute();
  MicrofacetDielectricReflectionAlbedoTable::precompute();
  MicrofacetSheenAlbedoTable::precompute();

  // Tables with energy compensation must be precomputed last
  MicrofacetDielectricReflectionECAlbedoTable::precompute();
  MicrofacetConductorArtisticECAlbedoTable::precompute();
}

} // namespace prt
