// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sys/memory.h"
#include "ray.h"
#include "ray_simd.h"
#include "bsdf/optics.h"

namespace prt {

constexpr int maxNumUVs = 2;

class ShadingContext
{
public:
  Vec3f p;             // position
  Basis3f f;           // shading frame
  Vec3f Ng;            // geometric normal
  Vec2f uv[maxNumUVs]; // texture coords
  bool backfacing;     // is the geometry backfacing?
  float eps;           // intersection epsilon

private:
  // Dynamically allocated data
  static constexpr int dataSize = 4096;
  char* ptr;
  prt_align_simd char data[dataSize];

public:
  prt_inline void begin()
  {
    ptr = data;
  }

  template <class T, class... Args>
  prt_inline T* make(Args&&... args)
  {
    T* obj = (T*)ptr;
    new (obj) T(std::forward<Args>(args)...);
    ptr += sizeof(T);
    return obj;
  }

  prt_inline Basis3f* makeFrame(const Vec3f& normal, const Vec3f& wo)
  {
    Basis3f& frame = *make<Basis3f>();

    // Compute perturbed normal
    Vec3f N = getFrame() * normal;

    // Local shading normal adaption
    // If the perfect reflection is below the surface, pull up the shading normal towards the geometric normal
    // This fixes some potential lighting artifacts (black spots)
    // [Keller et al., 2017, "The Iray Light Transport Simulation and Rendering System]
    Vec3f wi = reflect(wo, N);
    if (dot(wi, Ng) < 0.f)
    {
      // Pull up the reflection vector to be just above the geometry surface
      float a = -dot(wi, Ng);
      float b = dot(N, Ng);
      if (b < 0.f)
      {
        b = -b;
        wi = -wi;
      }
      Vec3f wi1 = normalize(wi + a*rcpSafe(b)*N + 1e-5f*Ng);

      // Recompute the shading normal using the adjusted reflection vector
      N = normalize(wo + wi1);
    }

    // Compute new frame using modified Gram-Schmidt
    frame.N = N;
    frame.U = f.U - dot(f.U, N) * N;
    float Ulen2 = lengthSqr(frame.U);
    if (Ulen2 > 0.f)
    {
      frame.U *= rsqrt(Ulen2); // normalize
      frame.V = f.V - dot(f.V, N) * N;
      frame.V -= dot(frame.V, frame.U) * frame.U;
      float Vlen2 = lengthSqr(frame.V);
      if (Vlen2 > 0.f)
        frame.V *= rsqrt(Vlen2); // normalize
      else
        prt::makeFrame(frame.U, frame.V, frame.N);
    }
    else
    {
      prt::makeFrame(frame.U, frame.V, frame.N);
    }

    return &frame;
  }

  prt_inline Basis3f* makeFrame(const Vec3f& wo)
  {
    return makeFrame(Vec3f(0.f, 0.f, 1.f), wo);
  }

  prt_inline const Basis3f& getFrame() const
  {
    return f;
  }

  prt_inline const Vec3f& getN() const
  {
    return f.N;
  }
};

class ShadingContextSimd
{
public:
  Vec3vf p;             // position
  Basis3vf f;           // shading frame
  Vec3vf Ng;            // geometric normal
  Vec2vf uv[maxNumUVs]; // texture coords
  vbool backfacing;     // is the geometry backfacing?
  vfloat eps;           // intersection epsilon

private:
  // Dynamically allocated data
  static constexpr int dataSize = PRT_SIMD_SIZE*PRT_SIMD_SIZE*1024;
  char* ptr;
  prt_align_simd char data[dataSize];

public:
  prt_inline void begin()
  {
    ptr = data;
  }

  template <class T, class... Args>
  prt_inline T* make(Args&&... args)
  {
    T* obj = (T*)ptr;
    new (obj) T(std::forward<Args>(args)...);
    ptr += sizeof(T);
    return obj;
  }

  prt_inline Basis3vf* makeFrame(vbool m, const Vec3vf& normal, const Vec3vf& wo)
  {
    Basis3vf& frame = *make<Basis3vf>();

    // Compute perturbed normal
    Vec3vf N = getFrame() * normal;

    // Local shading normal adaption
    // If the perfect reflection is below the surface, pull up the shading normal towards the geometric normal
    // This fixes some potential lighting artifacts (black spots)
    // [Keller et al., 2017, "The Iray Light Transport Simulation and Rendering System]
    Vec3vf wi = reflect(wo, N);
    vbool mFixN = m & (dot(wi, Ng) < 0.f);
    if (any(mFixN))
    {
      // Pull up the reflection vector to be just above the geometry surface
      vfloat a = -dot(wi, Ng);
      vfloat b = dot(N, Ng);
      vbool mNeg = b < 0.f;
      set(mNeg, b, -b);
      set(mNeg, wi, -wi);
      Vec3vf wi1 = normalize(wi + a*rcpSafe(b)*N + vfloat(1e-5f)*Ng);

      // Recompute the shading normal using the adjusted reflection vector
      set(mFixN, N, normalize(wo + wi1));
    }

    // Compute new frame (Gram-Schmidt)
    frame.N = N;
    frame.U = f.U - dot(f.U, N) * N;
    vfloat Ulen2 = lengthSqr(frame.U);
    frame.U *= rsqrt(Ulen2); // normalize
    frame.V = f.V - dot(f.V, N) * N;
    frame.V -= dot(frame.V, frame.U) * frame.U;
    vfloat Vlen2 = lengthSqr(frame.V);
    frame.V *= rsqrt(Vlen2); // normalize

    vbool mFixFrame = m & ((Ulen2 == 0.f) | (Vlen2 == 0.f));
    if (any(mFixFrame))
    {
      Vec3vf U, V;
      prt::makeFrame(U, V, frame.N);
      set(mFixFrame, frame.U, U);
      set(mFixFrame, frame.V, V);
    }

    return &frame;
  }

  prt_inline Basis3vf* makeFrame(vbool m, const Vec3vf& wo)
  {
    return makeFrame(m, Vec3vf(0.f, 0.f, 1.f), wo);
  }

  prt_inline const Basis3vf& getFrame() const
  {
    return f;
  }

  prt_inline const Vec3vf& getN() const
  {
    return f.N;
  }
};

class SimpleShadingContext
{
public:
  Vec3f p;   // position
  Vec3f Ng;  // geometric normal
  float eps; // intersection epsilon

  prt_inline Basis3f getFrame() const
  {
    return makeFrame(Ng);
  }

  prt_inline Vec3f getN() const
  {
    return Ng;
  }
};

class SimpleShadingContextSimd
{
public:
  Vec3vf p;   // position
  Vec3vf Ng;  // geometric normal
  vfloat eps; // intersection epsilon

  prt_inline Basis3vf getFrame() const
  {
    return makeFrame(Ng);
  }

  prt_inline Vec3vf getN() const
  {
    return Ng;
  }
};

} // namespace prt
