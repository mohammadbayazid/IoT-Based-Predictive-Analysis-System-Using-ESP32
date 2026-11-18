#ifndef LINEAR_REGRESSION_H
#define LINEAR_REGRESSION_H

#include <stddef.h>

/**
 * @brief Lightweight Ordinary Least Squares (OLS) Linear Regression model
 *        with L2 (Ridge) stabilization and circular buffer storage for ESP32.
 *
 * Fits y = slope * x + intercept over a rolling window of sensor readings,
 * where x is the sequential sample step [0 .. n-1], and predicts future
 * values k steps ahead with physical range clamping.
 */
class LinearRegression {
public:
  LinearRegression(size_t windowSize = 48,
                   float ridgeLambda = 1.0f,
                   float minBound = -100.0f,
                   float maxBound = 200.0f);

  /**
   * @brief Add a new sensor observation to the rolling window.
   * @param value Latest sensor reading (e.g., temperature or humidity).
   * @param autoFit If true, immediately recomputes regression parameters.
   */
  void addSample(float value, bool autoFit = true);

  /**
   * @brief Recompute slope, intercept, and R^2 over the current buffer.
   */
  void fit();

  /**
   * @brief Predict the value `stepsAhead` samples into the future.
   * @param stepsAhead Number of future steps from the latest sample.
   * @return Predicted value clamped to [minBound, maxBound].
   */
  float predict(float stepsAhead = 24.0f) const;

  /**
   * @brief Reset buffer and model parameters.
   */
  void reset();

  /**
   * @brief Retrieve the most recent raw observation stored in the buffer.
   */
  float getLatestSample() const;

  size_t getSampleCount() const { return count_; }
  float getSlope() const { return slope_; }
  float getIntercept() const { return intercept_; }
  float getRSquared() const { return rSquared_; }
  bool isTrained() const { return trained_; }

private:
  static const size_t MAX_CAPACITY = 64;

  float buffer_[MAX_CAPACITY];
  size_t capacity_;
  size_t head_;
  size_t count_;

  float ridgeLambda_;
  float minBound_;
  float maxBound_;

  float slope_;
  float intercept_;
  float rSquared_;
  bool trained_;

  float getOrderedSample(size_t index) const;
};

#endif // LINEAR_REGRESSION_H
