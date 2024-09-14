#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include "windowed_running_gradient.h"
#include "running_quadratic_gradient.h"
#include "rls_analysis_parameters.h"

// #define DEBUG_GRADIENT_CALC

/**
 * @brief Structure to store running gradient data for a quadratic model.
 *
 * This structure is used to maintain the data for a quadratic model, which includes the coefficients for the quadratic term,
 * linear term, and intercept.
 */
typedef struct {
    uint16_t num_points; /**< Number of data points currently stored */
    uint16_t max_points; /**< Maximum number of data points that can be stored */
    double x[RLS_WINDOW]; /**< Array of x values */
    double y[RLS_WINDOW]; /**< Array of y values */
    double coefficients[3]; /**< Coefficients of the quadratic regression model (a2, a1, a0) */
    double residual_sum_squares; /**< Residual sum of squares for the regression */
    double inverse_cov_matrix[3][3]; /**< Inverse covariance matrix for RLS update */
    double forgetting_factor; /**< Forgetting factor for the RLS algorithm */
} RunningQuadraticGradient;

/**
 * @brief Initializes the RunningQuadraticGradient structure.
 *
 * This function sets the initial values for the coefficients, residual sum of squares,
 * inverse covariance matrix, and allocates memory for x and y arrays for the quadratic model.
 *
 * @param rg Pointer to the RunningQuadraticGradient structure to initialize.
 * @param forgetting_factor Forgetting factor for the RLS algorithm.
 */
void init_running_quadratic_gradient(RunningQuadraticGradient *rg, double forgetting_factor) {
	rg->num_points = 0;
	rg->max_points = RLS_WINDOW;
	
	// Initialize coefficients to zero
	rg->coefficients[0] = 0.0;
	rg->coefficients[1] = 0.0;
	rg->coefficients[2] = 0.0;
	
	// Initialize residual sum of squares to zero
	rg->residual_sum_squares = 0.0;
	
	// Set the forgetting factor
	rg->forgetting_factor = forgetting_factor;
	
	// Initialize the inverse covariance matrix
	memset(rg->inverse_cov_matrix, 0, sizeof(rg->inverse_cov_matrix));
	rg->inverse_cov_matrix[0][0] = 1e9; // High uncertainty in x^2 coefficient
	rg->inverse_cov_matrix[1][1] = 1e9; // High uncertainty in x coefficient
	rg->inverse_cov_matrix[2][2] = 1e9; // High uncertainty in constant term
}

/**
 * @brief Adds a new data point to the RunningQuadraticGradient structure and updates the quadratic regression model using Recursive Least Squares (RLS).
 *
 * This function adds a new data point to the RunningQuadraticGradient structure, which maintains a quadratic model of the data points
 * using the Recursive Least Squares (RLS) algorithm to update the regression coefficients.
 *
 * @param rg Pointer to the RunningQuadraticGradient structure.
 * @param y The new data point to be added.
 */
void add_quadratic_data_point(RunningQuadraticGradient *const rg, const MqsRawDataPoint_t *data_point) {
    double y = data_point->phaseAngle; // Extract the phase angle from the data point

    // Check if the current window is not full
    if (rg->num_points < rg->max_points) {
        // Add new data point to the window
        rg->y[rg->num_points] = y;
        rg->x[rg->num_points] = (double)rg->num_points;
        rg->num_points++;
    } else {
        // Shift the window to the left and add new data point
        // This shift operation removes the oldest data point and adds the new one at the end.
        for (uint16_t i = 0; i < rg->max_points - 1; ++i) {
            rg->y[i] = rg->y[i + 1];
            rg->x[i] = rg->x[i + 1];
        }
        rg->y[rg->max_points - 1] = y;  // Add new data point
        rg->x[rg->max_points - 1] = (double)(rg->num_points);
        rg->num_points++;
    }

    // Define the input vector for the new data point
    // The input vector for quadratic fitting is x^2, x, and 1 (for the intercept).
    double x0 = pow((double)(rg->num_points - 1), 2); // Quadratic term
    double x1 = (double)(rg->num_points - 1); // Linear term
    double x2 = 1.0; // Constant term

    // Calculate the denominator term in the RLS update equations
    // This term helps in updating the inverse covariance matrix efficiently using the Sherman-Morrison formula.
    double temp = rg->forgetting_factor +
                  (x0 * rg->inverse_cov_matrix[0][0] + x1 * rg->inverse_cov_matrix[1][0] + x2 * rg->inverse_cov_matrix[2][0]) * x0 +
                  (x0 * rg->inverse_cov_matrix[0][1] + x1 * rg->inverse_cov_matrix[1][1] + x2 * rg->inverse_cov_matrix[2][1]) * x1 +
                  (x0 * rg->inverse_cov_matrix[0][2] + x1 * rg->inverse_cov_matrix[1][2] + x2 * rg->inverse_cov_matrix[2][2]) * x2;

    // Check for numerical stability
    if (fabs(temp) < 1e-10) {
        fprintf(stderr, "Numerical stability issue: temp is too close to zero.\n");
        exit(EXIT_FAILURE);
    }

    // Calculate the temporary vector used for updating the inverse covariance matrix
    double tmp0 = rg->inverse_cov_matrix[0][0] * x0 + rg->inverse_cov_matrix[0][1] * x1 + rg->inverse_cov_matrix[0][2] * x2;
    double tmp1 = rg->inverse_cov_matrix[1][0] * x0 + rg->inverse_cov_matrix[1][1] * x1 + rg->inverse_cov_matrix[1][2] * x2;
    double tmp2 = rg->inverse_cov_matrix[2][0] * x0 + rg->inverse_cov_matrix[2][1] * x1 + rg->inverse_cov_matrix[2][2] * x2;

    // Update the inverse covariance matrix using the Sherman-Morrison formula
    // The following lines update the inverse covariance matrix to incorporate the new data point.
    rg->inverse_cov_matrix[0][0] = (rg->inverse_cov_matrix[0][0] - (tmp0 * tmp0) / temp) / rg->forgetting_factor;
    rg->inverse_cov_matrix[0][1] = (rg->inverse_cov_matrix[0][1] - (tmp0 * tmp1) / temp) / rg->forgetting_factor;
    rg->inverse_cov_matrix[0][2] = (rg->inverse_cov_matrix[0][2] - (tmp0 * tmp2) / temp) / rg->forgetting_factor;
    rg->inverse_cov_matrix[1][0] = rg->inverse_cov_matrix[0][1]; // Enforcing symmetry
    rg->inverse_cov_matrix[1][1] = (rg->inverse_cov_matrix[1][1] - (tmp1 * tmp1) / temp) / rg->forgetting_factor;
    rg->inverse_cov_matrix[1][2] = (rg->inverse_cov_matrix[1][2] - (tmp1 * tmp2) / temp) / rg->forgetting_factor;
    rg->inverse_cov_matrix[2][0] = rg->inverse_cov_matrix[0][2]; // Enforcing symmetry
    rg->inverse_cov_matrix[2][1] = rg->inverse_cov_matrix[1][2]; // Enforcing symmetry
    rg->inverse_cov_matrix[2][2] = (rg->inverse_cov_matrix[2][2] - (tmp2 * tmp2) / temp) / rg->forgetting_factor;

    // Calculate the prediction error for the new data point
    // Prediction error is the difference between the actual data point and the value predicted by the quadratic model.
    double prediction_error = y - (x0 * rg->coefficients[0] + x1 * rg->coefficients[1] + x2 * rg->coefficients[2]);

    // Update the regression coefficients using the RLS update equations
    // The coefficients for the quadratic model are updated using the inverse covariance matrix and the prediction error.
    rg->coefficients[0] += (rg->inverse_cov_matrix[0][0] * x0 + rg->inverse_cov_matrix[0][1] * x1 + rg->inverse_cov_matrix[0][2] * x2) * prediction_error;
    rg->coefficients[1] += (rg->inverse_cov_matrix[1][0] * x0 + rg->inverse_cov_matrix[1][1] * x1 + rg->inverse_cov_matrix[1][2] * x2) * prediction_error;
    rg->coefficients[2] += (rg->inverse_cov_matrix[2][0] * x0 + rg->inverse_cov_matrix[2][1] * x1 + rg->inverse_cov_matrix[2][2] * x2) * prediction_error;

    // Recalculate the residual sum of squares
    // The residual sum of squares (RSS) is a measure of the discrepancy between the data and the estimation model.
    rg->residual_sum_squares = 0.0;
    for (uint16_t i = 0; i < rg->num_points; ++i) {
        double error = rg->y[i] - (rg->coefficients[0] * pow(rg->x[i], 2) + rg->coefficients[1] * rg->x[i] + rg->coefficients[2]);
        rg->residual_sum_squares += error * error;
    }
}


/**
 * @brief Calculates the slope (first derivative) of the quadratic function at a specific point.
 *
 * This function computes the slope of the quadratic equation represented by the
 * coefficients stored in the RunningQuadraticGradient structure at a given value of x.
 * The slope is derived from the first derivative of the quadratic function:
 *
 *     slope = 2 * a2 * x + a1
 *
 * where:
 *     a2 is the coefficient of the x^2 term,
 *     a1 is the coefficient of the x term,
 *     x is the point at which the slope is calculated.
 *
 * @param rg Pointer to the RunningQuadraticGradient structure containing the coefficients.
 * @param x The point at which to calculate the slope.
 * @return The slope (first derivative) of the quadratic function at the given point x.
 */
double calculate_slope_at_point(const RunningQuadraticGradient *rg, double x) {
	double a2 = rg->coefficients[0];  // Coefficient for x^2
	double a1 = rg->coefficients[1];  // Coefficient for x
	// Calculate the slope at the given point x
	double slope = 2.0 * a2 * x + a1;
	return slope;
}

/**
 * @brief Calculates the second-order gradient (curvature) of the quadratic function.
 *
 * This function computes the second-order gradient of the quadratic equation represented by the
 * coefficients stored in the RunningQuadraticGradient structure. The second-order gradient is
 * the second derivative of the quadratic function, which for a quadratic function is constant:
 *
 *     second_order_gradient = 2 * a2
 *
 * where:
 *     a2 is the coefficient of the x^2 term.
 *
 * @param rg Pointer to the RunningQuadraticGradient structure containing the coefficients.
 * @return The second-order gradient (curvature) of the quadratic function.
 */
double calculate_second_order_gradient(const RunningQuadraticGradient *rg) {
	double a2 = rg->coefficients[0];  // Coefficient for x^2
	double second_order_gradient = 2.0 * a2;
	return second_order_gradient;
}

/**
 * @brief Verifies the detected peak by checking for a consistent trend of increases on the left side
 * and decreases on the right side, with additional checks and adjustments for truncated data using quadratic RLS.
 *
 * This function verifies whether a detected peak is a true peak by analyzing the second-order gradients
 * calculated using quadratic regression. Specifically, it checks for a consistent increasing trend on the left
 * side and a decreasing trend on the right side of the peak.
 *
 * @param values Array of data points.
 * @param length Length of the data array.
 * @param second_order_gradients Array containing the precomputed second-order gradients.
 * @param peak_index The index of the detected peak within the second_order_gradients array.
 * @param start_index The starting index in the original data array corresponding to the first element of second_order_gradients.
 * @param forgetting_factor The forgetting factor used in the RLS algorithm.
 * @return bool True if the peak is verified based on the trend analysis, false otherwise.
 */
bool verify_quadratic_peak(const MqsRawDataPoint_t *values, uint16_t length, const double *second_order_gradients, uint16_t peak_index, uint16_t start_index, double forgetting_factor) {
    uint16_t left_trend_count = 0;
    uint16_t right_trend_count = 0;
    uint16_t inconsistency_count_left = 0;
    uint16_t inconsistency_count_right = 0;

    // Count increasing trends on the left side of the peak
    for (int i = peak_index; i > 0; --i) {
        if (second_order_gradients[i - 1] > 0) {
            left_trend_count++;
            if (left_trend_count >= MINIMUM_REQUIRED_TREND_COUNT) break;
        } else {
            inconsistency_count_left++;
            if (inconsistency_count_left > ALLOWABLE_INCONSISTENCY_COUNT) {
                break;
            }
        }
    }

    // Count decreasing trends on the right side of the peak
    for (uint16_t i = peak_index; i < RLS_WINDOW - 1; ++i) {
        if (second_order_gradients[i + 1] < 0) {
            right_trend_count++;
            if (right_trend_count >= MINIMUM_REQUIRED_TREND_COUNT) break;
        } else {
            inconsistency_count_right++;
            if (inconsistency_count_right > ALLOWABLE_INCONSISTENCY_COUNT) {
                break;
            }
        }
    }

    printf("Left trends count: %u, Right trends count: %u\n", left_trend_count, right_trend_count);

    // Verify if the peak meets the criteria
    return left_trend_count >= MINIMUM_REQUIRED_TREND_COUNT && right_trend_count >= MINIMUM_REQUIRED_TREND_COUNT;
}

/**
 * @brief Finds and verifies a peak in the data using quadratic RLS.
 *
 * This function first detects a peak using second-order gradients calculated from quadratic regression,
 * and then verifies the peak using the `verify_quadratic_peak` function. The peak is considered valid
 * only if it meets the trend criteria on both sides of the peak.
 *
 * @param values Array of data points.
 * @param length Length of the data array.
 * @param start_index The start index in the data array from which to begin the gradient calculation.
 * @param forgetting_factor The forgetting factor used in the RLS algorithm.
 * @return QuadraticPeakAnalysisResult Structure containing the peak detection status and the peak index if found and verified.
 */
QuadraticPeakAnalysisResult find_and_verify_quadratic_peak(const MqsRawDataPoint_t *values, uint16_t length, uint16_t start_index, double forgetting_factor) {
    QuadraticPeakAnalysisResult result = { .peak_found = false, .peak_index = 0 };

    // Declare the second_order_gradients array
    double second_order_gradients[RLS_WINDOW];

    // Initialize the running quadratic gradient structure
    RunningQuadraticGradient rg;
    init_running_quadratic_gradient(&rg, forgetting_factor);

    // Compute the second-order gradients
    for (uint16_t i = 0; i < RLS_WINDOW && (start_index + i) < length; ++i) {
        const MqsRawDataPoint_t *current_value = &values[start_index + i];
        add_quadratic_data_point(&rg, current_value);

        // Calculate the second-order gradient only if we have at least 3 data points
        if (rg.num_points >= 3) {
            double second_order_gradient = calculate_second_order_gradient(&rg);
            second_order_gradients[i] = second_order_gradient;
        } else {
            second_order_gradients[i] = NAN;  // Not enough points yet to calculate the gradient
        }
    }

    // Find and verify the peak based on the computed gradients
    for (uint16_t i = 1; i < RLS_WINDOW; ++i) {
        if (second_order_gradients[i - 1] > 0 && second_order_gradients[i] < 0) {
            // Temporarily set the peak index
            result.peak_index = start_index + i;

            // Verify the detected peak
            if (verify_quadratic_peak(values, length, second_order_gradients, i, start_index, forgetting_factor)) {
                result.peak_found = true;
                printf("Verified peak found at index %u\n", result.peak_index);
                break; // Exit the loop once a verified peak is found
            } else {
                printf("Peak at index %u did not pass verification. Continuing search...\n", result.peak_index);
                result.peak_found = false; // Reset peak_found as the peak failed verification
            }
        }
    }

    if (!result.peak_found) {
        printf("No verified peak found in the specified window.\n");
    }

    return result;
}

/**
 * @brief Computes the second-order gradients for the next 30 values in the given array.
 *
 * This function takes an array of doubles and a starting index, and then adds each of the next 30 values
 * to the RunningQuadraticGradient structure. After each addition, it computes the second-order gradient
 * (curvature) of the quadratic model. The function returns an array containing these second-order gradients.
 *
 * @param values Array of double values.
 * @param length Length of the array.
 * @param start_index Starting index from which to begin the gradient calculation.
 * @param forgetting_factor The forgetting factor for the RLS algorithm.
 * @return An array of double containing the second-order gradients.
 */
double compute_total_second_order_gradient(const MqsRawDataPoint_t *values, uint16_t length, uint16_t start_index, double forgetting_factor) {
    // Ensure that the start index and the length allow for RLS_WINDOW calculations
    if (start_index + RLS_WINDOW > length) {
        printf("Insufficient data to compute %d second-order gradients from the starting index.\n", RLS_WINDOW);
        return NAN;
    }

    // Initialize the running quadratic gradient structure
    RunningQuadraticGradient rg;
    init_running_quadratic_gradient(&rg, forgetting_factor);

    double total_sum_gradients = 0.0; // Initialize total sum of gradients
   
    // Loop through the next RLS_WINDOW values starting from start_index
    for (uint16_t i = 0; i < RLS_WINDOW; ++i) {
        const MqsRawDataPoint_t *current_value = &values[start_index + i];

        // Add the current value's phaseAngle to the running quadratic gradient
        add_quadratic_data_point(&rg, current_value);

        // Calculate the second-order gradient only if we have at least 3 data points
        if (rg.num_points >= 3) {
            double second_order_gradient = calculate_second_order_gradient(&rg);

            // Accumulate the total sum of gradients
            total_sum_gradients += second_order_gradient;

            // Optionally store the gradient in scratch space (if needed elsewhere in the function)
            //second_order_gradients[i] = second_order_gradient;

            // Optionally, print the total sum of gradients as it is adding new values
            //printf("Total sum of gradients after adding phase angle %.6f: %.6f\n", current_value->phaseAngle, total_sum_gradients);

            // Optionally, print the current second-order gradient
            //printf("Second-order gradient after adding phase angle %.6f: %.6f\n", current_value->phaseAngle, second_order_gradient);
        } else {
            // Optionally, handle the case when not enough data points are available
            //second_order_gradients[i] = NAN; // Not enough points yet to calculate the gradient

            // Optionally, print a message indicating insufficient data
            //printf("Not enough data points to calculate the gradient after adding phase angle %.6f.\n", current_value->phaseAngle);
        }
    }

    // Return the total sum of the second-order gradients
    return total_sum_gradients;
}

/**
 * @brief Finds a consistent increasing trend in the second-order gradient array.
 *
 * This function scans through an array of second-order gradient values to identify a consistent increasing trend.
 * It starts tracking when positive gradients are encountered and stops if more than one negative gradient is found consecutively.
 * The function returns a `GradientTrendIndices` struct containing the start and end indices of the consistent increase, 
 * a validity flag, and the cumulative sum of the gradients within this range.
 *
 * @param gradients The array of second-order gradient values to analyze.
 * @param start_index The starting index in the original data array from which the gradient array begins.
 * @param window_size The number of elements in the gradient array to analyze.
 * @return GradientTrendIndices A structure containing the start index, end index, validity, and maximum sum of the consistent increasing trend.
 */
GradientTrendIndices find_consistent_increase_in_second_order(double *gradients, size_t start_index, size_t window_size) {
    GradientTrendIndices increase_info = {0, 0, false, 0.0};
    bool tracking_increase = false;
    double cumulative_sum = 0.0;
    int decrease_count = 0;
    double max_cumulative_sum = 0.0;  // Track the maximum cumulative sum encountered
    size_t best_start_index = 0;  // Track the start index for the best trend
    size_t best_end_index = 0;    // Track the end index for the best trend

    #ifdef DEBUG
    printf("Starting find_consistent_increase_in_second_order with start_index: %zu, window_size: %zu\n", start_index, window_size);
    #endif

    for (size_t i = 0; i < window_size; ++i) {
        double gradient = gradients[i];

        #ifdef DEBUG
        printf("Index: %zu, Gradient: %.6f\n", start_index + i, gradient);
        #endif

        if (gradient > 0) {  // If the current gradient is positive
            if (!tracking_increase) {
                // Start or resume tracking the increase
                increase_info.start_index = start_index + i;
                tracking_increase = true;
                cumulative_sum = 0.0;  // Reset cumulative sum when starting to track

                #ifdef DEBUG
                printf("Started tracking increase at index %zu\n", start_index + i);
                #endif
            }
            increase_info.end_index = start_index + i;
            cumulative_sum += gradient;
            decrease_count = 0;  // Reset decrease counter on positive gradient

            // Update the best cumulative sum and indices
            if (cumulative_sum > max_cumulative_sum) {
                max_cumulative_sum = cumulative_sum;
                best_start_index = increase_info.start_index;
                best_end_index = increase_info.end_index;
            }

            #ifdef DEBUG
            printf("Continuing tracking increase: Updated end_index to %zu, Cumulative Sum: %.6f\n", start_index + i, cumulative_sum);
            #endif
        } else if (gradient < 0) {  // If the current gradient is negative
            decrease_count++;

            #ifdef DEBUG
            printf("Negative gradient found, Decrease Count: %d\n", decrease_count);
            #endif

            if (decrease_count > quadratic_analysis_params.max_second_order_trend_decrease_count) {  // Use global parameter
                tracking_increase = false;  // Stop tracking but do not discard the tracked increase

                #ifdef DEBUG
                printf("Stopped tracking increase due to consecutive decreases at index %zu\n", start_index + i);
                #endif

                // Reset decrease count and continue to look for the next increase
                decrease_count = 0;
            }
        }
    }

    // Use the best cumulative sum and indices found during tracking
    if (max_cumulative_sum > 0) {
        increase_info.valid = true;
        increase_info.max_sum = max_cumulative_sum;
        increase_info.start_index = best_start_index;
        increase_info.end_index = best_end_index;

        printf("Valid increase found from index %u to %u with max sum %.6f\n", increase_info.start_index, increase_info.end_index, max_cumulative_sum);
    } else {
        printf("No valid increase found.\n");
    }

    return increase_info;
}

/**
 * @brief Finds a consistent decreasing trend in the second-order gradient array.
 *
 * This function scans through an array of second-order gradient values to identify a consistent decreasing trend.
 * It starts tracking when negative gradients are encountered and stops if more than one positive gradient is found consecutively.
 * The function returns a `GradientTrendIndices` struct containing the start and end indices of the consistent decrease, 
 * a validity flag, and the cumulative sum of the gradients within this range.
 *
 * @param gradients The array of second-order gradient values to analyze.
 * @param start_index The starting index in the original data array from which the gradient array begins.
 * @param window_size The number of elements in the gradient array to analyze.
 * @return GradientTrendIndices A structure containing the start index, end index, validity, and maximum sum of the consistent decreasing trend.
 */
GradientTrendIndices find_consistent_decrease_in_second_order(double *gradients, size_t start_index, size_t window_size) {
    GradientTrendIndices decrease_info = {0, 0, false, 0.0};
    bool tracking_decrease = false;
    double cumulative_sum = 0.0;
    int increase_count = 0;

    #ifdef DEBUG
    printf("Starting find_consistent_increase_in_second_order with start_index: %u, window_size: %u\n", start_index, window_size);
    #endif

    for (size_t i = 0; i < window_size; ++i) {
        double gradient = gradients[i];

        #ifdef DEBUG
        printf("Index: %zu, Gradient: %.6f\n", start_index + i, gradient);
        #endif

        if (gradient < 0) {  // If the current gradient is negative
            if (!tracking_decrease) {
                // Start tracking the decrease
                decrease_info.start_index = start_index + i;
                tracking_decrease = true;
                cumulative_sum = 0.0;  // Reset cumulative sum when starting to track

                #ifdef DEBUG
                printf("Started tracking decrease at index %zu\n", start_index + i);
                #endif
            }
            decrease_info.end_index = start_index + i;
            cumulative_sum += gradient;
            increase_count = 0;  // Reset increase counter on negative gradient

            #ifdef DEBUG
            printf("Continuing tracking decrease: Updated end_index to %zu, Cumulative Sum: %.6f\n", start_index + i, cumulative_sum);
            #endif
        } else if (gradient > 0) {  // If the current gradient is positive
            increase_count++;

            #ifdef DEBUG
            printf("Positive gradient found, Increase Count: %d\n", increase_count);
            #endif

            if (increase_count > quadratic_analysis_params.max_second_order_trend_increase_count) {  // Use global parameter
                tracking_decrease = false;  // Stop tracking

                #ifdef DEBUG
                printf("Stopped tracking decrease due to consecutive increases at index %zu\n", start_index + i);
                #endif
                increase_count = 0;
                //break;
            }
        }

        // If tracking was stopped due to increases and a new decrease is found
        if (!tracking_decrease && gradient < 0 && increase_count > quadratic_analysis_params.max_second_order_trend_increase_count) {
            // Restart tracking from this new decrease
            decrease_info.start_index = start_index + i;
            tracking_decrease = true;
            cumulative_sum = gradient;  // Reset cumulative sum with the new decrease
            decrease_info.end_index = start_index + i;
            increase_count = 0;  // Reset the increase counter

            #ifdef DEBUG
            printf("Restarted tracking decrease at index %zu, Cumulative Sum: %.6f\n", start_index + i, cumulative_sum);
            #endif
        }
    }

    if (cumulative_sum < 0) {  // Ensure there's a valid decrease before marking it valid
        decrease_info.valid = true;
        decrease_info.max_sum = cumulative_sum;

       
        printf("Valid decrease found from index %u to %u with max sum %.6f\n", decrease_info.start_index, decrease_info.end_index, cumulative_sum);

    } else {

        printf("No valid decrease found.\n");

    }

    return decrease_info;
}

/**
 * @brief Tracks the second-order gradient trends within a specified window using quadratic regression.
 *
 * This function tracks the gradient trends of a given dataset within a specified window using quadratic regression.
 * It calculates second-order gradients (curvatures) at each point in the window, identifies regions of consistent increase,
 * and stores the maximum cumulative sum of gradients. It returns a structure containing the start and end indices
 * of the consistent increase, along with a flag indicating if the trend was valid and the maximum sum of gradients.
 *
 * @param values Array of double values representing the data points.
 * @param length The length of the values array.
 * @param start_index The starting index in the values array from which to begin the gradient calculation.
 * @param window_size The number of points to include in the gradient calculation.
 * @param forgetting_factor The forgetting factor used in the Recursive Least Squares (RLS) algorithm, typically close to 1,
 *        which determines the weight given to newer data points.
 * @return GradientTrendResult A struct containing:
 * - increase_info: Information about the detected increasing trend.
 * - decrease_info: Information about the detected decreasing trend.
 */
GradientTrendResult track_gradient_trends_with_quadratic_regression(const MqsRawDataPoint_t *values, uint16_t length, uint16_t start_index, uint16_t window_size, double forgetting_factor) {
    GradientTrendResult trend_result = {0};  // Initialize the struct with default values
    
    printf("track_gradient_trends_with_quadratic_regression called with arguments:\n");
    //printf("  length: %u\n", length);
    //printf("  start_index: %u\n", start_index);
    //printf("  window_size: %u\n", window_size);
    //printf("  forgetting_factor: %.6f\n", forgetting_factor);

    // Ensure that the start index and the window size allow for calculations
    if (start_index + window_size > length) {
        #ifdef DEBUG
        printf("Insufficient data to compute gradients for the specified window size.\n");
        #endif
        return trend_result;
    }

    // Array to store the second-order gradients
    double second_order_gradients[RLS_WINDOW];

    // Initialize the running quadratic gradient structure
    RunningQuadraticGradient rg;
    init_running_quadratic_gradient(&rg, forgetting_factor);

    // Loop through the specified window size and calculate second-order gradients
    for (uint16_t i = 0; i < window_size; ++i) {
        uint16_t current_index = start_index + i;
        const MqsRawDataPoint_t *current_value = &values[current_index];

        // Add the current value's phaseAngle to the running quadratic gradient
        add_quadratic_data_point(&rg, current_value);

        // Check if we have enough points to calculate the gradient
        if (rg.num_points >= 3) {
            double second_order_gradient = calculate_second_order_gradient(&rg);
            second_order_gradients[i] = second_order_gradient;

            // printf("Second-order gradient at index %u: %.6f\n", current_index, second_order_gradient);
        } else {
            second_order_gradients[i] = NAN;  // Not enough points yet to calculate the gradient

            #ifdef DEBUG
            printf("Not enough data points to calculate the gradient at index %u\n", current_index);
            #endif
        }
    }

    // Find the consistent increase trend
    #ifdef DEBUG
    printf("Finding consistent increase...\n");
    #endif
    trend_result.increase_info = find_consistent_increase_in_second_order(second_order_gradients, start_index, window_size);

    // Find the consistent decrease trend
    #ifdef DEBUG
    printf("Finding consistent decrease...\n");
    #endif
    trend_result.decrease_info = find_consistent_decrease_in_second_order(second_order_gradients, start_index, window_size);

    return trend_result;
}

