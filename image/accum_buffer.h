// Copyright 2015 Intel Corporation
// Copyright 2011-2022 Blender Foundation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sys/memory.h"
#include "sys/tasking.h"
#include "math/vec2.h"
#include "math/vec4.h"
#include "math/simd.h"
#include "pixel.h"
#include "color.h"
#include "surface.h"
#include "tone_mapper.h"

namespace prt {

namespace AccumFlag
{
  enum : int
  {
    None = 0,

    // Variance buffers
    // [Rousselle et al., 2012, "Adaptive Rendering with Non-Local Means Filtering"]
    SampleVar  = 1 << 1, // sample variance
    HalfBuffer = 1 << 2, // half buffer for two-buffer pixel (sample mean) variance

    CheckpointBuffer = 1 << 3, // copy of the buffer at the previous power-of-two sample count
  };
}

// Splits samples into two different classes, A and B, which can be compared for
// variance estimation
inline bool isSampleClassB(int index)
{
  /* This follows the approach from section 10.2.1 of "Progressive
   * Multi-Jittered Sample Sequences" by Christensen et al., but
   * implemented with efficient bit-fiddling.
   *
   * This approach also turns out to work equally well with Owen
   * scrambled and shuffled Sobol (see https://developer.blender.org/D15746#429471).
   */
  return bitCount(index & 0xaaaaaaaa) & 1;
}

template <int N>
class AccumBuffer : Uncopyable
{
protected:
  float* sum[N];
  float* weight;
  float* weightB; // class B samples only = half buffer
  float* sumB[N]; // class B samples only = half buffer
  float* sum2[N]; // squared samples, for sample variance
  float* sumC[N];
  float* weightC;
  int count;      // number of accumulated samples
  bool classB;
  Vec2i size;

  void init(const Vec2i& size, int flags)
  {
    this->size = size;

    for (int i = 0; i < N; ++i)
      sum[i] = (float*)alignedAlloc(size.x * size.y * sizeof(float));

    if (flags & AccumFlag::HalfBuffer)
    {
      weightB = (float*)alignedAlloc(size.x * size.y * sizeof(float));
      for (int i = 0; i < N; ++i)
        sumB[i] = (float*)alignedAlloc(size.x * size.y * sizeof(float));
    }
    else
    {
      weightB = nullptr;
      for (int i = 0; i < N; ++i)
        sumB[i] = nullptr;
    }

    if (flags & AccumFlag::SampleVar)
    {
      for (int i = 0; i < N; ++i)
        sum2[i] = (float*)alignedAlloc(size.x * size.y * sizeof(float));
    }
    else
    {
      for (int i = 0; i < N; ++i)
        sum2[i] = nullptr;
    }

    if (flags & AccumFlag::CheckpointBuffer)
    {
      weightC = (float*)alignedAlloc(size.x * size.y * sizeof(float));
      for (int i = 0; i < N; ++i)
        sumC[i] = (float*)alignedAlloc(size.x * size.y * sizeof(float));
    }
    else
    {
      weightC = nullptr;
      for (int i = 0; i < N; ++i)
        sumC[i] = nullptr;
    }

    weight = (float*)alignedAlloc(size.x * size.y * sizeof(float));

    clear();
  }

public:
  AccumBuffer()
  {
    for (int i = 0; i < N; ++i)
    {
      sum[i] = nullptr;
      sumB[i] = nullptr;
      sum2[i] = nullptr;
      sumC[i] = nullptr;
    }

    weight = nullptr;
    weightB = nullptr;
    weightC = nullptr;
    count = 0;
    classB = isSampleClassB(count);
  }

  AccumBuffer(const Vec2i& size, int flags)
  {
    init(size, flags);
  }

  ~AccumBuffer()
  {
    free();
  }

  void alloc(const Vec2i& size, int flags = 0)
  {
    free();
    init(size, flags);
  }

  void free()
  {
    for (int i = 0; i < N; ++i)
    {
      alignedFree(sum[i]);
      alignedFree(sumB[i]);
      alignedFree(sum2[i]);
      alignedFree(sumC[i]);
    }
    alignedFree(weight);
    alignedFree(weightB);
    alignedFree(weightC);
  }

  prt_inline Vec2i getSize() const
  {
    return size;
  }

  prt_inline operator bool() const
  {
    return sum[0] != nullptr;
  }

  prt_inline bool hasSampleVar() const
  {
    return sum2[0] != nullptr;
  }

  prt_inline bool hasHalfBuffer() const
  {
    return sumB[0] != nullptr;
  }

  prt_inline bool hasCheckpointBuffer() const
  {
    return sumC[0] != nullptr;
  }

  void clear()
  {
    count = 0;
    classB = isSampleClassB(count);

    if (sum[0] == nullptr)
      return;

    tbb::parallel_for(tbb::blocked_range<int>(0, size.x*size.y), [&](const tbb::blocked_range<int>& r)
    {
      for (int i = 0; i < N; ++i)
      {
        for (int j = r.begin(); j != r.end(); ++j)
          sum[i][j] = 0.f;
      }
    });

    if (hasHalfBuffer())
    {
      tbb::parallel_for(tbb::blocked_range<int>(0, size.x*size.y), [&](const tbb::blocked_range<int>& r)
      {
        for (int j = r.begin(); j != r.end(); ++j)
          weightB[j] = 0.f;

        for (int i = 0; i < N; ++i)
        {
          for (int j = r.begin(); j != r.end(); ++j)
            sumB[i][j] = 0.f;
        }
      });
    }

    if (hasSampleVar())
    {
      tbb::parallel_for(tbb::blocked_range<int>(0, size.x*size.y), [&](const tbb::blocked_range<int>& r)
      {
        for (int i = 0; i < N; ++i)
        {
          for (int j = r.begin(); j != r.end(); ++j)
            sum2[i][j] = 0.f;
        }
      });
    }

    if (hasCheckpointBuffer())
    {
      tbb::parallel_for(tbb::blocked_range<int>(0, size.x*size.y), [&](const tbb::blocked_range<int>& r)
      {
        for (int j = r.begin(); j != r.end(); ++j)
          weightC[j] = 0.f;

        for (int i = 0; i < N; ++i)
        {
          for (int j = r.begin(); j != r.end(); ++j)
            sumC[i][j] = 0.f;
        }
      });
    }

    tbb::parallel_for(tbb::blocked_range<int>(0, size.x*size.y), [&](const tbb::blocked_range<int>& r)
    {
      for (int j = r.begin(); j != r.end(); ++j)
        weight[j] = 0.f;
    });
  }

  // Call before starting to accumulate pixels
  void begin()
  {
    if (hasCheckpointBuffer() && isPowerOfTwo(count))
    {
      memcpy(weightC, weight, size.x * size.y * sizeof(float));
      for (int i = 0; i < N; ++i)
        memcpy(sumC[i], sum[i], size.x * size.y * sizeof(float));
    }
  }

  // Call when all pixels have been accumulated
  void finish()
  {
    count++;
    classB = isSampleClassB(count);
  }
};

class AccumBuffer3f : public AccumBuffer<3>
{
public:
  AccumBuffer3f() {}
  AccumBuffer3f(const Vec2i& size, int flags = 0) : AccumBuffer(size, flags) {}

  prt_inline void add(const Vec2i& p, const Vec3f& c)
  {
    const int index = p.y * size.x + p.x;

    float x = sum[0][index] + c[0];
    float y = sum[1][index] + c[1];
    float z = sum[2][index] + c[2];
    float w = weight[index] + 1.f;

    sum[0][index] = x;
    sum[1][index] = y;
    sum[2][index] = z;
    weight[index] = w;

    if (hasHalfBuffer() && classB)
    {
      float xB = sumB[0][index] + c[0];
      float yB = sumB[1][index] + c[1];
      float zB = sumB[2][index] + c[2];
      float wB = weightB[index] + 1.f;

      sumB[0][index] = xB;
      sumB[1][index] = yB;
      sumB[2][index] = zB;
      weightB[index] = wB;
    }

    if (hasSampleVar())
    {
      float x2 = sum2[0][index] + sqr(c[0]);
      float y2 = sum2[1][index] + sqr(c[1]);
      float z2 = sum2[2][index] + sqr(c[2]);

      sum2[0][index] = x2;
      sum2[1][index] = y2;
      sum2[2][index] = z2;
    }
  }

  prt_inline void add(const Vec2vi& p, const Vec3vf& c)
  {
    const vint index = p.y * size.x + p.x;

    vfloat x = gather(sum[0], index) + c[0];
    vfloat y = gather(sum[1], index) + c[1];
    vfloat z = gather(sum[2], index) + c[2];
    vfloat w = gather(weight, index) + 1.f;

    scatter(sum[0], index, x);
    scatter(sum[1], index, y);
    scatter(sum[2], index, z);
    scatter(weight, index, w);

    if (hasHalfBuffer() && classB)
    {
      vfloat xB = gather(sumB[0], index) + c[0];
      vfloat yB = gather(sumB[1], index) + c[1];
      vfloat zB = gather(sumB[2], index) + c[2];
      vfloat wB = gather(weightB, index) + 1.f;

      scatter(sumB[0], index, xB);
      scatter(sumB[1], index, yB);
      scatter(sumB[2], index, zB);
      scatter(weightB, index, wB);
    }

    if (hasSampleVar())
    {
      vfloat x2 = gather(sum2[0], index) + sqr(c[0]);
      vfloat y2 = gather(sum2[1], index) + sqr(c[1]);
      vfloat z2 = gather(sum2[2], index) + sqr(c[2]);

      scatter(sum2[0], index, x2);
      scatter(sum2[1], index, y2);
      scatter(sum2[2], index, z2);
    }
  }

  prt_inline void add(vbool m, const Vec2vi& p, const Vec3vf& c)
  {
    const vint index = p.y * size.x + p.x;

    vfloat x = gather(m, sum[0], index) + c[0];
    vfloat y = gather(m, sum[1], index) + c[1];
    vfloat z = gather(m, sum[2], index) + c[2];
    vfloat w = gather(m, weight, index) + 1.f;

    scatter(m, sum[0], index, x);
    scatter(m, sum[1], index, y);
    scatter(m, sum[2], index, z);
    scatter(m, weight, index, w);

    if (hasHalfBuffer() && classB)
    {
      vfloat xB = gather(m, sumB[0], index) + c[0];
      vfloat yB = gather(m, sumB[1], index) + c[1];
      vfloat zB = gather(m, sumB[2], index) + c[2];
      vfloat wB = gather(m, weightB, index) + 1.f;

      scatter(m, sumB[0], index, xB);
      scatter(m, sumB[1], index, yB);
      scatter(m, sumB[2], index, zB);
      scatter(m, weightB, index, wB);
    }

    if (hasSampleVar())
    {
      vfloat x2 = gather(m, sum2[0], index) + sqr(c[0]);
      vfloat y2 = gather(m, sum2[1], index) + sqr(c[1]);
      vfloat z2 = gather(m, sum2[2], index) + sqr(c[2]);

      scatter(m, sum2[0], index, x2);
      scatter(m, sum2[1], index, y2);
      scatter(m, sum2[2], index, z2);
    }
  }

  // Serial version for duplicate indices
  prt_inline void addSerial(const Vec2vi& p, const Vec3vf& c)
  {
    const vint index = p.y * size.x + p.x;

    #pragma unroll
    for (int i = 0; i < simdSize; ++i)
    {
      int indexI = index[i];

      float x = sum[0][indexI] + c[0][i];
      float y = sum[1][indexI] + c[1][i];
      float z = sum[2][indexI] + c[2][i];
      float w = weight[indexI] + 1.f;

      sum[0][indexI] = x;
      sum[1][indexI] = y;
      sum[2][indexI] = z;
      weight[indexI] = w;

      if (hasHalfBuffer() && classB)
      {
        float xB = sumB[0][indexI] + c[0][i];
        float yB = sumB[1][indexI] + c[1][i];
        float zB = sumB[2][indexI] + c[2][i];
        float wB = weightB[indexI] + 1.f;

        sumB[0][indexI] = xB;
        sumB[1][indexI] = yB;
        sumB[2][indexI] = zB;
        weightB[indexI] = wB;
      }

      if (hasSampleVar())
      {
        float x2 = sum2[0][indexI] + sqr(c[0][i]);
        float y2 = sum2[1][indexI] + sqr(c[1][i]);
        float z2 = sum2[2][indexI] + sqr(c[2][i]);

        sum2[0][indexI] = x2;
        sum2[1][indexI] = y2;
        sum2[2][indexI] = z2;
      }
    }
  }

  prt_inline void addSerial(vbool m, const Vec2vi& p, const Vec3vf& c)
  {
    const vint index = p.y * size.x + p.x;
    const int mInt = toIntMask(m);

    #pragma unroll
    for (int i = 0; i < simdSize; ++i)
    {
      if (mInt & (1 << i))
      {
        int indexI = index[i];

        float x = sum[0][indexI] + c[0][i];
        float y = sum[1][indexI] + c[1][i];
        float z = sum[2][indexI] + c[2][i];
        float w = weight[indexI] + 1.f;

        sum[0][indexI] = x;
        sum[1][indexI] = y;
        sum[2][indexI] = z;
        weight[indexI] = w;

        if (hasHalfBuffer() && classB)
        {
          float xB = sumB[0][indexI] + c[0][i];
          float yB = sumB[1][indexI] + c[1][i];
          float zB = sumB[2][indexI] + c[2][i];
          float wB = weightB[indexI] + 1.f;

          sumB[0][indexI] = xB;
          sumB[1][indexI] = yB;
          sumB[2][indexI] = zB;
          weightB[indexI] = wB;
        }

        if (hasSampleVar())
        {
          float x2 = sum2[0][indexI] + sqr(c[0][i]);
          float y2 = sum2[1][indexI] + sqr(c[1][i]);
          float z2 = sum2[2][indexI] + sqr(c[2][i]);

          sum2[0][indexI] = x2;
          sum2[1][indexI] = y2;
          sum2[2][indexI] = z2;
        }
      }
    }
  }

  prt_inline void addAtomic(vbool m, const Vec2vi& p, const Vec3vf& c, vfloat cw = 1.f)
  {
    const vint index = p.y * size.x + p.x;
    const int mInt = toIntMask(m);

    #pragma unroll
    for (int i = 0; i < simdSize; ++i)
    {
      if (mInt & (1 << i))
      {
        int indexI = index[i];

        atomicAdd(&sum[0][indexI], c[0][i]);
        atomicAdd(&sum[1][indexI], c[1][i]);
        atomicAdd(&sum[2][indexI], c[2][i]);
        atomicAdd(&weight[indexI], cw[i]);

        if (hasHalfBuffer() && classB)
        {
          atomicAdd(&sumB[0][indexI], c[0][i]);
          atomicAdd(&sumB[1][indexI], c[1][i]);
          atomicAdd(&sumB[2][indexI], c[2][i]);
          atomicAdd(&weightB[indexI], cw[i]);
        }

        if (hasSampleVar())
        {
          atomicAdd(&sum2[0][indexI], sqr(c[0][i]));
          atomicAdd(&sum2[1][indexI], sqr(c[1][i]));
          atomicAdd(&sum2[2][indexI], sqr(c[2][i]));
        }
      }
    }
  }

  prt_inline void set(const Vec2i& p, const Vec3f& c)
  {
    const int index = p.y * size.x + p.x;

    sum[0][index] = c[0];
    sum[1][index] = c[1];
    sum[2][index] = c[2];
    weight[index] = 1.f;

    if (hasHalfBuffer())
    {
      sumB[0][index] = 0.f;
      sumB[1][index] = 0.f;
      sumB[2][index] = 0.f;
    }

    if (hasSampleVar())
    {
      sum2[0][index] = sqr(c[0]);
      sum2[1][index] = sqr(c[1]);
      sum2[2][index] = sqr(c[2]);
    }
  }

  prt_inline void set(const Vec2vi& p, const Vec3vf& c)
  {
    const vint index = p.y * size.x + p.x;

    scatter(sum[0], index, c[0]);
    scatter(sum[1], index, c[1]);
    scatter(sum[2], index, c[2]);
    scatter(weight, index, 1.f);

    if (hasHalfBuffer())
    {
      scatter(sumB[0], index, zero);
      scatter(sumB[1], index, zero);
      scatter(sumB[2], index, zero);
      scatter(weightB, index, zero);
    }

    if (hasSampleVar())
    {
      scatter(sum2[0], index, sqr(c[0]));
      scatter(sum2[1], index, sqr(c[1]));
      scatter(sum2[2], index, sqr(c[2]));
    }
  }

  prt_inline void set(vbool m, const Vec2vi& p, const Vec3vf& c)
  {
    const vint index = p.y * size.x + p.x;

    scatter(m, sum[0], index, c[0]);
    scatter(m, sum[1], index, c[1]);
    scatter(m, sum[2], index, c[2]);
    scatter(m, weight, index, 1.f);

    if (hasHalfBuffer())
    {
      scatter(m, sumB[0], index, zero);
      scatter(m, sumB[1], index, zero);
      scatter(m, sumB[2], index, zero);
      scatter(m, weightB, index, zero);
    }

    if (hasSampleVar())
    {
      scatter(m, sum2[0], index, sqr(c[0]));
      scatter(m, sum2[1], index, sqr(c[1]));
      scatter(m, sum2[2], index, sqr(c[2]));
    }
  }

  prt_inline Vec3vf get(int index) const
  {
    vfloat x = load(sum[0] + index);
    vfloat y = load(sum[1] + index);
    vfloat z = load(sum[2] + index);
    vfloat w = load(weight + index);

    return Vec3vf(x, y, z) * rcpSafe(w);
  }

  void blit(Vec3f* dest, Vec3f* destVar = nullptr, Vec3f* destHalf = nullptr) const
  {
    if (destVar && !hasSampleVar())
      throw std::logic_error("sample variance not enabled for the buffer");
    if (destHalf && !hasHalfBuffer())
      throw std::logic_error("half buffer not enabled for the buffer");

    tbb::parallel_for(tbb::blocked_range<int>(0, size.x*size.y), [&](const tbb::blocked_range<int>& r)
    {
      for (int i = r.begin(); i != r.end(); ++i)
      {
        float x = sum[0][i];
        float y = sum[1][i];
        float z = sum[2][i];
        float w = weight[i];

        Vec3f mu = Vec3f(x, y, z) * rcpSafe(w);
        dest[i] = mu;

        if (destVar)
        {
          float x2 = sum2[0][i];
          float y2 = sum2[1][i];
          float z2 = sum2[2][i];

          Vec3f s2 = max(Vec3f(x2, y2, z2) - sqr(Vec3f(x, y, z)) * rcpSafe(w), Vec3f(0.f)) * rcpSafe(max(w - 1.f, 1.f));
          destVar[i] = s2;
        }

        if (destHalf)
        {
          float xB = sumB[0][i];
          float yB = sumB[1][i];
          float zB = sumB[2][i];
          float wB = weightB[i];

          Vec3f B = Vec3f(xB, yB, zB) * rcpSafe(wB);
          destHalf[i] = B;
        }
      }
    });
  }

  // Blits tone-mapped colors
  void blitLdr(Surface& dest, ToneMapper* toneMapper = nullptr) const
  {
    tbb::parallel_for(tbb::blocked_range<int>(0, size.y), [&](const tbb::blocked_range<int>& r)
    {
      for (int iy = r.begin(); iy != r.end(); ++iy)
      {
        int i = iy * size.x;
        int* outRow = dest.getRow(iy);

        for (int ix = 0; ix < size.x; ix += simdSize)
        {
          vfloat x = load(sum[0] + i);
          vfloat y = load(sum[1] + i);
          vfloat z = load(sum[2] + i);
          vfloat w = load(weight + i);

          Vec3vf mu = Vec3vf(x, y, z) * rcpSafe(w);

          if (toneMapper)
            mu = toneMapper->get(mu);

          store(outRow + ix, encodeBgr8(mu));
          i += simdSize;
        }
      }
    });
  }

  void blitLdr(Vec3f* dest, ToneMapper* toneMapper = nullptr, Vec3f* destHalf = nullptr) const
  {
    if (destHalf && !hasHalfBuffer())
      throw std::logic_error("half buffer not enabled for the buffer");

    tbb::parallel_for(tbb::blocked_range<int>(0, size.y), [&](const tbb::blocked_range<int>& r)
    {
      for (int iy = r.begin(); iy != r.end(); ++iy)
      {
        int i = iy * size.x;

        for (int ix = 0; ix < size.x; ix += simdSize)
        {
          vfloat x = load(sum[0] + i);
          vfloat y = load(sum[1] + i);
          vfloat z = load(sum[2] + i);
          vfloat w = load(weight + i);

          Vec3vf mu = Vec3vf(x, y, z) * rcpSafe(w);

          if (toneMapper)
            mu = toneMapper->get(mu);

          mu = clamp(mu, vfloat(0.f), vfloat(1.f));

          for (int k = 0; k < simdSize; ++k)
            dest[i + k] = Vec3f(mu.x[k], mu.y[k], mu.z[k]);

          if (destHalf)
          {
            vfloat xB = load(sumB[0] + i);
            vfloat yB = load(sumB[1] + i);
            vfloat zB = load(sumB[2] + i);
            vfloat wB = load(weightB + i);

            Vec3vf B = Vec3vf(xB, yB, zB) * rcpSafe(wB);

            if (toneMapper)
            {
              //A = toneMapper->get(A);
              B = toneMapper->get(B);
            }

            B = clamp(B, vfloat(0.f), vfloat(1.f));

            for (int k = 0; k < simdSize; ++k)
              destHalf[i + k] = Vec3f(B.x[k], B.y[k], B.z[k]);
          }

          i += simdSize;
        }
      }
    });
  }

  // Blits relative values
  void blitRel(Vec3f* dest, const Vec3f* srcDiv) const
  {
    tbb::parallel_for(tbb::blocked_range<int>(0, size.x*size.y), [&](const tbb::blocked_range<int>& r)
    {
      for (int i = r.begin(); i != r.end(); ++i)
      {
        float x = sum[0][i];
        float y = sum[1][i];
        float z = sum[2][i];
        float w = weight[i];

        Vec3f mu = Vec3f(x, y, z) * rcpSafe(w);
        dest[i] = mu / max(srcDiv[i], Vec3f(1e-4f));
      }
    });
  }

  float getError(ToneMapper* toneMapper = nullptr) const
  {
    if (!hasHalfBuffer() && !hasCheckpointBuffer())
      throw std::logic_error("half buffer or checkpoint buffer not enabled");

    double error = tbb::parallel_deterministic_reduce(
      tbb::blocked_range<int>(0, size.x*size.y),
      0.,
      [&](const tbb::blocked_range<int>& r, double error)
      {
        for (int i = r.begin(); i != r.end(); ++i)
        {
          float x = sum[0][i];
          float y = sum[1][i];
          float z = sum[2][i];
          float w = weight[i];
          Vec3f I = Vec3f(x, y, z) * rcpSafe(w);

          float xB, yB, zB, wB;
          if (hasCheckpointBuffer())
          {
            xB = sumC[0][i];
            yB = sumC[1][i];
            zB = sumC[2][i];
            wB = weightC[i];
          }
          else
          {
            xB = sumB[0][i];
            yB = sumB[1][i];
            zB = sumB[2][i];
            wB = weightB[i];
          }

          Vec3f B = Vec3f(xB, yB, zB) * rcpSafe(wB);

          if (toneMapper)
          {
            float scale = toneMapper->getExposure();
            I *= scale;
            B *= scale;
          }

          float norm = I.x + I.y + I.z;
          if (norm < 1.f)
            norm = sqrt(norm);
          error += (abs(I.x - B.x) + abs(I.y - B.y) + abs(I.z - B.z)) / (norm + 1e-4f);
        }
        return error;
      },
      std::plus<double>());

    return float(error / double(size.x*size.y));
  }
};

class AccumBuffer2f : public AccumBuffer<2>
{
public:
  AccumBuffer2f() {}
  AccumBuffer2f(const Vec2i& size, int flags = 0) : AccumBuffer(size, flags) {}

  prt_inline void add(const Vec2i& p, const Vec2f& c)
  {
    const int index = p.y * size.x + p.x;

    float x = sum[0][index] + c[0];
    float y = sum[1][index] + c[1];
    float w = weight[index] + 1.f;

    sum[0][index] = x;
    sum[1][index] = y;
    weight[index] = w;

    if (hasHalfBuffer() && classB)
    {
      float xB = sumB[0][index] + c[0];
      float yB = sumB[1][index] + c[1];
      float wB = weightB[index] + 1.f;

      sumB[0][index] = xB;
      sumB[1][index] = yB;
      weightB[index] = wB;
    }

    if (hasSampleVar())
    {
      float x2 = sum2[0][index] + sqr(c[0]);
      float y2 = sum2[1][index] + sqr(c[1]);

      sum2[0][index] = x2;
      sum2[1][index] = y2;
    }
  }

  prt_inline void add(const Vec2vi& p, const Vec2vf& c)
  {
    const vint index = p.y * size.x + p.x;

    vfloat x = gather(sum[0], index) + c[0];
    vfloat y = gather(sum[1], index) + c[1];
    vfloat w = gather(weight, index) + 1.f;

    scatter(sum[0], index, x);
    scatter(sum[1], index, y);
    scatter(weight, index, w);

    if (hasHalfBuffer() && classB)
    {
      vfloat xB = gather(sumB[0], index) + c[0];
      vfloat yB = gather(sumB[1], index) + c[1];
      vfloat wB = gather(weightB, index) + 1.f;

      scatter(sumB[0], index, xB);
      scatter(sumB[1], index, yB);
      scatter(weightB, index, wB);
    }

    if (hasSampleVar())
    {
      vfloat x2 = gather(sum2[0], index) + sqr(c[0]);
      vfloat y2 = gather(sum2[1], index) + sqr(c[1]);

      scatter(sum2[0], index, x2);
      scatter(sum2[1], index, y2);
    }
  }

  prt_inline void add(vbool m, const Vec2vi& p, const Vec2vf& c)
  {
    const vint index = p.y * size.x + p.x;

    vfloat x = gather(m, sum[0], index) + c[0];
    vfloat y = gather(m, sum[1], index) + c[1];
    vfloat w = gather(m, weight, index) + 1.f;

    scatter(m, sum[0], index, x);
    scatter(m, sum[1], index, y);
    scatter(m, weight, index, w);

    if (hasHalfBuffer() && classB)
    {
      vfloat xB = gather(m, sumB[0], index) + c[0];
      vfloat yB = gather(m, sumB[1], index) + c[1];
      vfloat wB = gather(m, weightB, index) + 1.f;

      scatter(m, sumB[0], index, xB);
      scatter(m, sumB[1], index, yB);
      scatter(m, weightB, index, wB);
    }

    if (hasSampleVar())
    {
      vfloat x2 = gather(m, sum2[0], index) + sqr(c[0]);
      vfloat y2 = gather(m, sum2[1], index) + sqr(c[1]);

      scatter(m, sum2[0], index, x2);
      scatter(m, sum2[1], index, y2);
    }
  }

  // Serial version for duplicate indices
  prt_inline void addSerial(const Vec2vi& p, const Vec2vf& c)
  {
    const vint index = p.y * size.x + p.x;

    #pragma unroll
    for (int i = 0; i < simdSize; ++i)
    {
      int indexI = index[i];

      float x = sum[0][indexI] + c[0][i];
      float y = sum[1][indexI] + c[1][i];
      float w = weight[indexI] + 1.f;

      sum[0][indexI] = x;
      sum[1][indexI] = y;
      weight[indexI] = w;

      if (hasHalfBuffer() && classB)
      {
        float xB = sumB[0][indexI] + c[0][i];
        float yB = sumB[1][indexI] + c[1][i];
        float wB = weightB[indexI] + 1.f;

        sumB[0][indexI] = xB;
        sumB[1][indexI] = yB;
        weightB[indexI] = wB;
      }

      if (hasSampleVar())
      {
        float x2 = sum2[0][indexI] + sqr(c[0][i]);
        float y2 = sum2[1][indexI] + sqr(c[1][i]);

        sum2[0][indexI] = x2;
        sum2[1][indexI] = y2;
      }
    }
  }

  prt_inline void addSerial(vbool m, const Vec2vi& p, const Vec2vf& c)
  {
    const vint index = p.y * size.x + p.x;
    const int mInt = toIntMask(m);

    #pragma unroll
    for (int i = 0; i < simdSize; ++i)
    {
      if (mInt & (1 << i))
      {
        int indexI = index[i];

        float x = sum[0][indexI] + c[0][i];
        float y = sum[1][indexI] + c[1][i];
        float w = weight[indexI] + 1.f;

        sum[0][indexI] = x;
        sum[1][indexI] = y;
        weight[indexI] = w;

        if (hasHalfBuffer() && classB)
        {
          float xB = sumB[0][indexI] + c[0][i];
          float yB = sumB[1][indexI] + c[1][i];
          float wB = weightB[indexI] + 1.f;

          sumB[0][indexI] = xB;
          sumB[1][indexI] = yB;
          weightB[indexI] = wB;
        }

        if (hasSampleVar())
        {
          float x2 = sum2[0][indexI] + sqr(c[0][i]);
          float y2 = sum2[1][indexI] + sqr(c[1][i]);

          sum2[0][indexI] = x2;
          sum2[1][indexI] = y2;
        }
      }
    }
  }

  prt_inline void addAtomic(vbool m, const Vec2vi& p, const Vec2vf& c, vfloat cw = 1.f)
  {
    const vint index = p.y * size.x + p.x;
    const int mInt = toIntMask(m);

    #pragma unroll
    for (int i = 0; i < simdSize; ++i)
    {
      if (mInt & (1 << i))
      {
        int indexI = index[i];

        atomicAdd(&sum[0][indexI], c[0][i]);
        atomicAdd(&sum[1][indexI], c[1][i]);
        atomicAdd(&weight[indexI], cw[i]);

        if (hasHalfBuffer() && classB)
        {
          atomicAdd(&sumB[0][indexI], c[0][i]);
          atomicAdd(&sumB[1][indexI], c[1][i]);
          atomicAdd(&weightB[indexI], cw[i]);
        }

        if (hasSampleVar())
        {
          atomicAdd(&sum2[0][indexI], sqr(c[0][i]));
          atomicAdd(&sum2[1][indexI], sqr(c[1][i]));
        }
      }
    }
  }

  prt_inline void set(const Vec2i& p, const Vec2f& c)
  {
    const int index = p.y * size.x + p.x;

    sum[0][index] = c[0];
    sum[1][index] = c[1];
    weight[index] = 1.f;

    if (hasHalfBuffer())
    {
      sumB[0][index] = 0.f;
      sumB[1][index] = 0.f;
    }

    if (hasSampleVar())
    {
      sum2[0][index] = sqr(c[0]);
      sum2[1][index] = sqr(c[1]);
    }
  }

  prt_inline void set(const Vec2vi& p, const Vec2vf& c)
  {
    const vint index = p.y * size.x + p.x;

    scatter(sum[0], index, c[0]);
    scatter(sum[1], index, c[1]);
    scatter(weight, index, 1.f);

    if (hasHalfBuffer())
    {
      scatter(sumB[0], index, zero);
      scatter(sumB[1], index, zero);
      scatter(weightB, index, zero);
    }

    if (hasSampleVar())
    {
      scatter(sum2[0], index, sqr(c[0]));
      scatter(sum2[1], index, sqr(c[1]));
    }
  }

  prt_inline void set(vbool m, const Vec2vi& p, const Vec2vf& c)
  {
    const vint index = p.y * size.x + p.x;

    scatter(m, sum[0], index, c[0]);
    scatter(m, sum[1], index, c[1]);
    scatter(m, weight, index, 1.f);

    if (hasHalfBuffer())
    {
      scatter(m, sumB[0], index, zero);
      scatter(m, sumB[1], index, zero);
      scatter(m, weightB, index, zero);
    }

    if (hasSampleVar())
    {
      scatter(m, sum2[0], index, sqr(c[0]));
      scatter(m, sum2[1], index, sqr(c[1]));
    }
  }

  prt_inline Vec2vf get(int index) const
  {
    vfloat x = load(sum[0] + index);
    vfloat y = load(sum[1] + index);
    vfloat w = load(weight + index);

    return Vec2vf(x, y) * rcpSafe(w);
  }

  void blit(Vec2f* dest, Vec2f* destVar = nullptr, Vec2f* destHalf = nullptr) const
  {
    if (destVar && !hasSampleVar())
      throw std::logic_error("sample variance not enabled for the buffer");
    if (destHalf && !hasHalfBuffer())
      throw std::logic_error("half buffer not enabled for the buffer");

    tbb::parallel_for(tbb::blocked_range<int>(0, size.x*size.y), [&](const tbb::blocked_range<int>& r)
    {
      for (int i = r.begin(); i != r.end(); ++i)
      {
        float x = sum[0][i];
        float y = sum[1][i];
        float w = weight[i];

        Vec2f mu = Vec2f(x, y) * rcpSafe(w);
        dest[i] = mu;

        if (destVar)
        {
          float x2 = sum2[0][i];
          float y2 = sum2[1][i];

          Vec2f s2 = max(Vec2f(x2, y2) - sqr(Vec2f(x, y)) * rcpSafe(w), Vec2f(0.f)) * rcpSafe(max(w - 1.f, 1.f));
          destVar[i] = s2;
        }

        if (destHalf)
        {
          float xB = sumB[0][i];
          float yB = sumB[1][i];
          float wB = weightB[i];

          Vec2f B = Vec2f(xB, yB) * rcpSafe(wB);
          destHalf[i] = B;
        }
      }
    });
  }
};

class AccumBuffer1f : public AccumBuffer<1>
{
public:
  AccumBuffer1f() {}
  AccumBuffer1f(const Vec2i& size, int flags = 0) : AccumBuffer(size, flags) {}

  prt_inline void add(const Vec2i& p, float c)
  {
    const int index = p.y * size.x + p.x;

    float x = sum[0][index] + c;
    float w = weight[index] + 1.f;

    sum[0][index] = x;
    weight[index] = w;

    if (hasHalfBuffer() && classB)
    {
      float xB = sumB[0][index] + c;
      float wB = weightB[index] + 1.f;

      sumB[0][index] = xB;
      weightB[index] = wB;
    }

    if (hasSampleVar())
    {
      float x2 = sum2[0][index] + sqr(c);
      sum2[0][index] = x2;
    }
  }

  prt_inline void add(const Vec2vi& p, vfloat c)
  {
    const vint index = p.y * size.x + p.x;

    vfloat x = gather(sum[0], index) + c;
    vfloat w = gather(weight, index) + 1.f;

    scatter(sum[0], index, x);
    scatter(weight, index, w);

    if (hasHalfBuffer() && classB)
    {
      vfloat xB = gather(sumB[0], index) + c;
      vfloat wB = gather(weightB, index) + 1.f;

      scatter(sumB[0], index, xB);
      scatter(weightB, index, wB);
    }

    if (hasSampleVar())
    {
      vfloat x2 = gather(sum2[0], index) + sqr(c);
      scatter(sum2[0], index, x2);
    }
  }

  prt_inline void add(vbool m, const Vec2vi& p, vfloat c)
  {
    const vint index = p.y * size.x + p.x;

    vfloat x = gather(m, sum[0], index) + c;
    vfloat w = gather(m, weight, index) + 1.f;

    scatter(m, sum[0], index, x);
    scatter(m, weight, index, w);

    if (hasHalfBuffer() && classB)
    {
      vfloat xB = gather(m, sumB[0], index) + c;
      vfloat wB = gather(m, weightB, index) + 1.f;

      scatter(m, sumB[0], index, xB);
      scatter(m, weightB, index, wB);
    }

    if (hasSampleVar())
    {
      vfloat x2 = gather(m, sum2[0], index) + sqr(c);
      scatter(m, sum2[0], index, x2);
    }
  }

  // Serial version for duplicate indices
  prt_inline void addSerial(const Vec2vi& p, vfloat c)
  {
    const vint index = p.y * size.x + p.x;

    #pragma unroll
    for (int i = 0; i < simdSize; ++i)
    {
      int indexI = index[i];

      float x = sum[0][indexI] + c[i];
      float w = weight[indexI] + 1.f;

      sum[0][indexI] = x;
      weight[indexI] = w;

      if (hasHalfBuffer() && classB)
      {
        float xB = sumB[0][indexI] + c[i];
        float wB = weightB[indexI] + 1.f;

        sumB[0][indexI] = xB;
        weightB[indexI] = wB;
      }

      if (hasSampleVar())
      {
        float x2 = sum2[0][indexI] + sqr(c[i]);
        sum2[0][indexI] = x2;
      }
    }
  }

  prt_inline void addSerial(vbool m, const Vec2vi& p, vfloat c)
  {
    const vint index = p.y * size.x + p.x;
    const int mInt = toIntMask(m);

    #pragma unroll
    for (int i = 0; i < simdSize; ++i)
    {
      if (mInt & (1 << i))
      {
        int indexI = index[i];

        float x = sum[0][indexI] + c[i];
        float w = weight[indexI] + 1.f;

        sum[0][indexI] = x;
        weight[indexI] = w;

        if (hasHalfBuffer() && classB)
        {
          float xB = sumB[0][indexI] + c[i];
          float wB = weightB[indexI] + 1.f;

          sumB[0][indexI] = xB;
          weightB[indexI] = wB;
        }

        if (hasSampleVar())
        {
          float x2 = sum2[0][indexI] + sqr(c[i]);
          sum2[0][indexI] = x2;
        }
      }
    }
  }

  prt_inline void addAtomic(vbool m, const Vec2vi& p, vfloat c, vfloat cw = 1.f)
  {
    const vint index = p.y * size.x + p.x;
    const int mInt = toIntMask(m);

    #pragma unroll
    for (int i = 0; i < simdSize; ++i)
    {
      if (mInt & (1 << i))
      {
        int indexI = index[i];

        atomicAdd(&sum[0][indexI], c[i]);
        atomicAdd(&weight[indexI], cw[i]);

        if (hasHalfBuffer() && classB)
        {
          atomicAdd(&sumB[0][indexI], c[i]);
          atomicAdd(&weightB[indexI], cw[i]);
        }

        if (hasSampleVar())
          atomicAdd(&sum2[0][indexI], sqr(c[i]));
      }
    }
  }

  prt_inline void set(const Vec2i& p, float c)
  {
    const int index = p.y * size.x + p.x;

    sum[0][index] = c;
    weight[index] = 1.f;

    if (hasHalfBuffer())
    {
      sumB[0][index] = 0.f;
      weightB[index] = 0.f;
    }

    if (hasSampleVar())
      sum2[0][index] = sqr(c);
  }

  prt_inline void set(const Vec2vi& p, vfloat c)
  {
    const vint index = p.y * size.x + p.x;

    scatter(sum[0], index, c);
    scatter(weight, index, 1.f);

    if (hasHalfBuffer())
    {
      scatter(sumB[0], index, zero);
      scatter(weightB, index, zero);
    }

    if (hasSampleVar())
      scatter(sum2[0], index, sqr(c));
  }

  prt_inline void set(vbool m, const Vec2vi& p, vfloat c)
  {
    const vint index = p.y * size.x + p.x;

    scatter(m, sum[0], index, c);
    scatter(m, weight, index, 1.f);

    if (hasHalfBuffer())
    {
      scatter(m, sumB[0], index, zero);
      scatter(m, weightB, index, zero);
    }

    if (hasSampleVar())
      scatter(m, sum2[0], index, sqr(c));
  }

  prt_inline vfloat get(int index) const
  {
    vfloat x = load(sum[0] + index);
    vfloat w = load(weight + index);

    return x * rcpSafe(w);
  }

  void blit(float* dest, float* destVar = nullptr, float* destHalf = nullptr) const
  {
    if (destVar && !hasSampleVar())
      throw std::logic_error("sample variance not enabled for the buffer");
    if (destHalf && !hasHalfBuffer())
      throw std::logic_error("half buffer not enabled for the buffer");

    tbb::parallel_for(tbb::blocked_range<int>(0, size.x*size.y), [&](const tbb::blocked_range<int>& r)
    {
      for (int i = r.begin(); i != r.end(); ++i)
      {
        float x = sum[0][i];
        float w = weight[i];

        float mu = x * rcpSafe(w);
        dest[i] = mu;

        if (destVar)
        {
          float x2 = sum2[0][i];

          float s2 = max(x2 - sqr(x) * rcpSafe(w), 0.f) * rcpSafe(max(w - 1.f, 1.f));
          destVar[i] = s2;
        }

        if (destHalf)
        {
          float xB = sumB[0][i];
          float wB = weightB[i];

          float B = xB * rcpSafe(wB);
          destHalf[i] = B;
        }
      }
    });
  }
};

} // namespace prt
