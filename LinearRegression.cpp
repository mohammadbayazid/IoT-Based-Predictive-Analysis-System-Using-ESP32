#include "LinearRegression.h"
#include <math.h>

LinearRegression::LinearRegression(size_t windowSize,
                                   float ridgeLambda,
                                   float minBound,
                                   float maxBound)
    : capacity_(windowSize > MAX_CAPACITY ? MAX_CAPACITY : (windowSize < 2 ? 2 : windowSize)),
      head_(0),
      count_(0),
      ridgeLambda_(ridgeLambda >= 0.0f ? ridgeLambda : 0.0f),
      minBound_(minBound),
      maxBound_(maxBound),
      slope_(0.0f),
      intercept_(0.0f),
      rSquared_(0.0f),
      trained_(false) {
  for (size_t i = 0; i < MAX_CAPACITY; ++i) {
    buffer_[i] = 0.0f;
  }
}

void LinearRegression::reset() {
  head_ = 0;
  count_ = 0;
  slope_ = 0.0f;
  intercept_ = 0.0f;
  rSquared_ = 0.0f;
  trained_ = false;
}

void LinearRegression::addSample(float value, bool autoFit) {
  if (isnan(value) || isinf(value)) {
    return;
  }

  buffer_[head_] = value;
  head_ = (head_ + 1) % capacity_;
  if (count_ < capacity_) {
    count_++;
  }

  if (autoFit) {
    fit();
  }
}

float LinearRegression::getOrderedSample(size_t index) const {
  if (count_ < capacity_) {
    return buffer_[index];
  }
  size_t physicalIndex = (head_ + index) % capacity_;
  return buffer_[physicalIndex];
}

float LinearRegression::getLatestSample() const {
  if (count_ == 0) {
    return 0.0f;
  }
  return getOrderedSample(count_ - 1);
}

void LinearRegression::fit() {
  if (count_ == 0) {
    trained_ = false;
    return;
  }

  if (count_ == 1) {
    slope_ = 0.0f;
    intercept_ = getOrderedSample(0);
    rSquared_ = 0.0f;
    trained_ = true;
    return;
  }

  const float n = static_cast<float>(count_);
  // Mean of x = 0, 1, ..., n - 1 is (n - 1) / 2
  const float meanX = (n - 1.0f) * 0.5f;

  float sumY = 0.0f;
  for (size_t i = 0; i < count_; ++i) {
    sumY += getOrderedSample(i);
  }
  const float meanY = sumY / n;

  float sumXY = 0.0f;
  float sumXX = 0.0f;
  float ssTot = 0.0f;

  for (size_t i = 0; i < count_; ++i) {
    const float dx = static_cast<float>(i) - meanX;
    const float dy = getOrderedSample(i) - meanY;
    sumXY += dx * dy;
    sumXX += dx * dx;
    ssTot += dy * dy;
  }

  // Ordinary Least Squares with optional Ridge (L2) stabilization
  const float denominator = sumXX + ridgeLambda_;
  if (denominator > 1e-6f) {
    slope_ = sumXY / denominator;
  } else {
    slope_ = 0.0f;
  }

  intercept_ = meanY - slope_ * meanX;

  // Compute R^2 (coefficient of determination)
  if (ssTot > 1e-6f) {
    float ssRes = 0.0f;
    for (size_t i = 0; i < count_; ++i) {
      const float yHat = slope_ * static_cast<float>(i) + intercept_;
      const float residual = getOrderedSample(i) - yHat;
      ssRes += residual * residual;
    }
    rSquared_ = 1.0f - (ssRes / ssTot);
    if (rSquared_ < 0.0f) rSquared_ = 0.0f;
    if (rSquared_ > 1.0f) rSquared_ = 1.0f;
  } else {
    // All observations are identical; zero-slope line fits perfectly
    rSquared_ = 1.0f;
  }

  trained_ = true;
}

float LinearRegression::predict(float stepsAhead) const {
  if (!trained_ || count_ == 0) {
    return 0.0f;
  }

  // Latest sample is at index (count_ - 1)
  const float targetX = static_cast<float>(count_ - 1) + stepsAhead;
  float predicted = slope_ * targetX + intercept_;

  if (predicted < minBound_) predicted = minBound_;
  if (predicted > maxBound_) predicted = maxBound_;

  return predicted;
}
