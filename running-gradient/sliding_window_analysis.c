#include "sliding_window_analysis.h"
#include <stdio.h>

/******************************************************************************/
/* Global Variables */
/******************************************************************************/

typedef union {
    uint8_t value;
    struct {
        uint8_t isPeakFound : 1;
        uint8_t isUndecided : 1;
        uint8_t isCentered : 1;           // New flag
        uint8_t isSweepRequested : 1;
        uint8_t isSweepDone : 1;
        uint8_t reserved : 3;             // Adjusted reserved bits
    };
} SwpStatus_t;

SlidingWindowAnalysisContext ctx; // Sliding window analysis context
SwpStatus_t currentStatus = { .value = 0x0 };  // Initialize all flags to 0
SwpState_t currentState = SWP_WAITING;  // Initialize to SWP_WAITING state

/******************************************************************************/
/* Function Prototypes (Internal Functions) */
/******************************************************************************/

// Function Declarations for OnEntry and OnExit functions
static void OnEntryInitialAnalysis(void);
static void OnEntrySegmentAnalysis(void);
static void OnEntryUpdateBufferDirection(void);
static void OnEntryUndecidedTrendCase(void);
static void OnEntryPeakFindingAnalysis(void);
static void OnEntryWaiting(void);

static void OnEntryPeakCentering(void);

static void OnExitInitialAnalysis(void);
static void OnExitSegmentAnalysis(void);
static void OnExitUpdateBufferDirection(void);
static void OnExitUndecidedTrendCase(void);
static void OnExitPeakFindingAnalysis(void);
static void OnExitWaiting(void);
static void OnExitPeakCentering(void);

static void initBufferManager(void);
static SwpState_t NextState(SwpState_t state);

/******************************************************************************/
/* State Function Definitions */
/******************************************************************************/
typedef struct {
    void (*onEntry)(void);
    void (*onExit)(void);
    bool isComplete;
} StateFuncs_t;

// Update the STATE_FUNCS array
static StateFuncs_t STATE_FUNCS[SWP_STATE_LAST] = {
    {OnEntryInitialAnalysis, OnExitInitialAnalysis, false},         // SWP_INITIAL_ANALYSIS
    {OnEntrySegmentAnalysis, OnExitSegmentAnalysis, false},         // SWP_SEGMENT_ANALYSIS
    {OnEntryUpdateBufferDirection, OnExitUpdateBufferDirection, false}, // SWP_UPDATE_BUFFER_DIRECTION
    {OnEntryUndecidedTrendCase, OnExitUndecidedTrendCase, false},   // SWP_UNDECIDED_TREND_CASE
    {OnEntryPeakCentering, OnExitPeakCentering, false},             // SWP_PEAK_CENTERING
    {OnEntryPeakFindingAnalysis, OnExitPeakFindingAnalysis, false}, // SWP_PEAK_FINDING_ANALYSIS
    {OnEntryWaiting, OnExitWaiting, false}                          // SWP_WAITING
};
/******************************************************************************/
/* Sliding Window Analysis Functions */
/******************************************************************************/
static void initBufferManager(void) {
    // Initialize the buffer manager with appropriate parameters
    // Arguments:
    // buffer          -> The buffer that will store the phase and impedance values (array of MqsRawDataPoint_t)
    // BUFFER_SIZE     -> Total size of the buffer (maximum number of data points the buffer can hold)
    // 30              -> Sliding window size (number of data points to analyze in each window)
    // 0               -> Starting index in the phaseAngles array (this could be any index where you want to start the analysis)
    // 11300.0         -> Starting frequency for the frequency sweep (in Hz, e.g., starting at 11300 Hz)
    // 1.0             -> Frequency increment per step (in Hz, e.g., increment by 1 Hz for each data point)
    int start_index = 220;
    init_buffer_manager(buffer, BUFFER_SIZE, WINDOW_SIZE, start_index, 11300.0, 1.0);

    // Debugging: Print initialization state
    //printf("[DEBUG] Buffer Manager initialized:\n");
    //printf("Buffer size: %d, Window size: %d, Start index: %d\n", BUFFER_SIZE, WINDOW_SIZE, start_index);
}

void startSlidingWindowAnalysis(const double* phaseAngles, uint16_t phase_angle_size, Callback_t callback) {
    ctx.phaseAngles = phaseAngles;
    ctx.phase_angle_size = phase_angle_size;
    ctx.callback = callback;

    // Initialize the buffer manager
    initBufferManager();
    
    	// detect_significant_gradient_trends, determine_trend_direction
	init_cubic_rls_analysis_parameters(                                                                                                        //CHECKS IF THERE IS A PEAK VIA COUNTING/SUMMING CONSISTENT GRADIENT INCREASES 
        7.0,   // significance_thresh: Threshold for determining significant cubic trends.                                                     BU DA ÇOK ÖNEMLİ AMA SAYISI ARTTIRABİLİR.
               // Trends with a sum of gradients above this threshold are considered significant.
        5,     // duration_thresh: Minimum number of consecutive points required for a trend to be considered significant.                     BUNUN ÇOK ÖNEMLİ OLDUĞU ORTAYA ÇIKTI. BU OLMAZSA PEAKİ YARISINDA KESEBİLİR SEARCH. 
               // Ensures that only sustained trends are analyzed.
        2,     // min_trend_count: Minimum number of consistent trends required for cubic analysis.
               // Helps filter out noise and minor fluctuations.
        2,     // max_third_order_trend_decrease_count: Maximum number of consecutive decreases allowed in a cubic trend.           
               // Allows for minor fluctuations without discarding a potentially valid trend.
        2      // max_third_order_trend_increase_count: Maximum number of consecutive increases allowed in a cubic trend.
               // Helps to filter out noise when identifying a cubic trend.
    );
    
    // Initialize the quadratic RLS analysis parameters with the necessary thresholds.                                                          CHECKS IF THERE IS ENOUGH CONCAVITY INCREASE TO SEE IF WE REALLY CAPTURED THE WHOLE PEAK. 
    // INCREASELERIN BŞALADIKLARI YERDEN YAP BUNU?!
    init_quadratic_rls_analysis_parameters(
        0.3,   // min_gradient_sum: Minimum cumulative sum of second-order gradients required for a trend to be considered valid.               //KULLANILMIYOR.HARDCODED 2.5 VALUE YERINE BUNU YAZABILIRIZ. SAYIYA DÖNÜŞTÜRÜLMESİ LAZIM. COUNT IT. (already doing, jut reflect it to checks)
               // Helps in identifying significant concave or convex regions.
        2,     // max_decrease_count: Maximum number of consecutive negative gradients allowed when tracking an increasing trend.               //SAYI SAYACAĞIMIZ İÇİN LAZIM. SIGNIFICANT INCREASE İÇİN. 
               // Allows for minor fluctuations without discarding a potentially valid trend.
        2      // max_increase_count: Maximum number of consecutive positive gradients allowed when tracking a decreasing trend.                //NEGATİF DURUMLARDA DA SAYI SAYACAĞIMIZ İÇİN LAZIM. SIGNIFICANT DECREASE ICIN. 
               // Helps to filter out noise when identifying a decreasing trend.
    );
    
    // Initialize the on-peak analysis parameters for average gradient thresholds and consistent trend count
   // // INCREASELERIN BŞALADIKLARI YERDEN YAP BUNU?!
    init_on_peak_analysis_parameters( 
        0.8,  // min_avg_increase: Minimum average increase required to consider a significant increasing trend. LEFT SIDE OF THE PEAK            //KULLANILIYOR MU?
               // Ensures that only trends with substantial average gradient are flagged as significant.
        -0.19, // min_avg_decrease: Minimum average decrease required to consider a significant decreasing trend. RIGHT SIDE OF THE PEAK          //KULLANILIYOR MU?
               // Ensures that only trends with substantial average gradient are flagged as significant.
        5      // min_consistent_trend_count: Minimum number of consecutive data points required for a trend to be considered consistent. 
               // Helps ensure that only sustained trends are analyzed.
    );
    
    // Initialize gradient analysis parameters
    init_gradient_analysis_params(
        0.1,  // gradient_thresh: Threshold for determining a significant gradient increase.                                                    //COMPARE GRADIENT PARTSIN UYESI. SADECE BUYUK GRADIENT ARTIŞLARINI SAYIYOR. 
        0.6   // min_gradient_total: Minimum total gradient to flag a significant trend.                                                        //BUNUN ALTINDAKİLERE UNDECIDED DİYORUZ. 
    );
    
        // Enable sweep request
    currentStatus.isSweepRequested = 1;  // Start sweep request

    // Set the initial state to waiting
    currentState = SWP_WAITING;

    // Set the initial state and start the state machine
    SwpProcessStateChange();  // Start the state machine
}

/******************************************************************************/
/* State Machine Entry Functions */
/******************************************************************************/
static void OnEntryInitialAnalysis(void) {
    //printf("→→→→→Entering SWP_INITIAL_ANALYSIS state.\n");

    load_initial_buffer(ctx.phaseAngles, ctx.phase_angle_size);
    
      // Debugging buffer state after load
    printf("[DEBUG] Buffer Manager State after initial loading:\n");
    printf("Current buffer index: %d\n", buffer_manager.current_buffer_index);
    printf("Current phase index: %d\n", buffer_manager.current_phase_index);
    //printf("Window size: %d\n", buffer_manager.window_size);
    //printf("Buffer size: %d\n", buffer_manager.buffer_size);
    currentStatus.isSweepRequested = true;
    STATE_FUNCS[SWP_INITIAL_ANALYSIS].isComplete = true;
    SwpProcessStateChange();  // Move to next state
}

static void OnEntrySegmentAnalysis(void) {
    printf("→→→→→Entering SWP_SEGMENT_ANALYSIS state.\n");
    
    float forgetting_factor = 0.5f;

    SegmentAnalysisResult result = segment_trend_and_concavity_analysis(
        &buffer_manager.buffer[buffer_manager.current_buffer_index],
        buffer_manager.window_size,
        forgetting_factor
    );

    ctx.direction = result.nextDirection;

    if (ctx.direction == ON_PEAK) {
        currentStatus.isPeakFound = 1;
    } else if (ctx.direction == UNDECIDED) {
        currentStatus.isUndecided = 1;
    }

    STATE_FUNCS[SWP_SEGMENT_ANALYSIS].isComplete = true;
    SwpProcessStateChange();
}

static void OnEntryUpdateBufferDirection(void) {
    // Pass the movement amount as an argument
    update_buffer_for_direction(ctx.phaseAngles, ctx.direction, buffer_manager.window_size / 2);

    STATE_FUNCS[SWP_UPDATE_BUFFER_DIRECTION].isComplete = true;
    SwpProcessStateChange();  // Move to next state
}

static void OnEntryUndecidedTrendCase(void) {  //ÜZERİNE YAZIYOR.
    //printf("→→→→→→Entering SWP_UNDECIDED_TREND_CASE state.\n");

    handle_undecided_case(ctx.phaseAngles, ctx.phase_angle_size);

    STATE_FUNCS[SWP_UNDECIDED_TREND_CASE].isComplete = true;
    SwpProcessStateChange();
}

static void OnEntryPeakCentering(void) {
    printf("→→→→→Entering SWP_PEAK_CENTERING state.\n");

    // Reset the isComplete flag
    STATE_FUNCS[SWP_PEAK_CENTERING].isComplete = false;

    // Get the start index in the buffer
    uint16_t start_index = buffer_manager.current_buffer_index;
    uint16_t window_size = buffer_manager.window_size;


    //    double total_gradient_sum = compute_total_second_order_gradient(test_buffer, size, start_index, 0.5);
    // Compute the total sum of second-order gradients using the buffer directly
    double total_gradient_sum = compute_total_second_order_gradient(
        buffer_manager.buffer,
        buffer_manager.buffer_size,
        start_index,
        0.5 // Forgetting factor
    );

    printf("Total sum of second-order gradients: %.6f\n", total_gradient_sum);

    // If total_gradient_sum <= 2.0, consider the peak centered
    if (total_gradient_sum <= 1.0) {
        currentStatus.isCentered = 1;
        printf("Peak is centered based on total_gradient_sum.\n");
        STATE_FUNCS[SWP_PEAK_CENTERING].isComplete = true;
    }

    // Else, we need to adjust the buffer to center the peak
    // Use track_gradient_trends_with_quadratic_regression to get trend info
    GradientTrendResult gradient_trends = track_gradient_trends_with_quadratic_regression(
        buffer_manager.buffer,
        buffer_manager.buffer_size,
        start_index,
        window_size,
        0.5 // Forgetting factor
    );

    // Check if both trends are valid
    if (!gradient_trends.increase_info.valid || !gradient_trends.decrease_info.valid) {
        printf("Invalid trend data. Cannot proceed with centering.\n");
        currentStatus.isCentered = 1; // Consider it centered for now
        STATE_FUNCS[SWP_PEAK_CENTERING].isComplete = true;
    }

    // Calculate durations of increase and decrease trends
    uint16_t increase_start = gradient_trends.increase_info.start_index;
    uint16_t increase_end = gradient_trends.increase_info.end_index;
    uint16_t decrease_start = gradient_trends.decrease_info.start_index;
    uint16_t decrease_end = gradient_trends.decrease_info.end_index;

    // Adjust indices relative to the buffer
    uint16_t increase_duration = (increase_end + buffer_manager.buffer_size - increase_start) % buffer_manager.buffer_size;
    uint16_t decrease_duration = (decrease_end + buffer_manager.buffer_size - decrease_start) % buffer_manager.buffer_size;

    int shift_amount = 0;
    int direction = UNDECIDED; // LEFT_SIDE or RIGHT_SIDE

    if (increase_duration > decrease_duration) {
        // Peak is to the left; we need to move right
        shift_amount = (increase_duration - decrease_duration);
        direction = RIGHT_SIDE;
        printf("Increase duration (%u) > decrease duration (%u). Moving right by %d.\n",
               increase_duration, decrease_duration, shift_amount);
               
               currentStatus.isCentered = 1;
    } else if (decrease_duration > increase_duration) {
        // Peak is to the right; we need to move left
        shift_amount = (decrease_duration - increase_duration);
        direction = LEFT_SIDE;
        printf("Decrease duration (%u) > increase duration (%u). Moving left by %d.\n",
               decrease_duration, increase_duration, shift_amount);
               
               currentStatus.isCentered = 1;
    } else {
        // Durations are equal; consider the peak centered
        currentStatus.isCentered = 1;
        printf("Increase and decrease durations are equal. Peak is centered.\n");
        STATE_FUNCS[SWP_PEAK_CENTERING].isComplete = true;
        
        currentStatus.isCentered = 1;
    }

    if (shift_amount == 0) {
        // No need to shift
        currentStatus.isCentered = 1;
        printf("No shift needed. Peak is centered.\n");
        STATE_FUNCS[SWP_PEAK_CENTERING].isComplete = true;
    }

    // Adjust the buffer
    update_buffer_for_direction(ctx.phaseAngles, direction, shift_amount);
    
    // Do not set isComplete to true; the state machine will re-enter this state
    SwpProcessStateChange();
}

static void OnEntryPeakFindingAnalysis(void) {
    //printf("----→Entering SWP_PEAK_FINDING_ANALYSIS state.\n");

    verify_peak_at_index(buffer_manager.current_buffer_index);

    STATE_FUNCS[SWP_PEAK_FINDING_ANALYSIS].isComplete = true;
    currentStatus.isSweepDone = 1;
    SwpProcessStateChange();
}

static void OnEntryWaiting(void) {
    //printf("→→→→→Entering SWP_WAITING state.\n");

    if (currentStatus.isSweepRequested) {
        STATE_FUNCS[SWP_WAITING].isComplete = true;
    }
}

/******************************************************************************/
/* Exit Functions */
/******************************************************************************/
static void OnExitInitialAnalysis(void) {
    //printf("Exiting SWP_INITIAL_ANALYSIS state.\n");
}

static void OnExitSegmentAnalysis(void) {
    //printf("Exiting SWP_SEGMENT_ANALYSIS state.\n");
}

static void OnExitUpdateBufferDirection(void) {
    //printf("Exiting SWP_UPDATE_BUFFER_DIRECTION state.\n");
}

static void OnExitUndecidedTrendCase(void) {
    //printf("Exiting SWP_UNDECIDED_TREND_CASE state.\n");
}

static void OnExitPeakFindingAnalysis(void) {
    //printf("Exiting SWP_PEAK_FINDING_ANALYSIS state.\n");
}

static void OnExitWaiting(void) {
    //printf("Exiting SWP_WAITING state.\n");
}

static void OnExitPeakCentering(void) {
    //printf("Exiting SWP_PEAK_CENTERING state.\n");
}

/******************************************************************************/
/* State Machine Process and Transitions */
/******************************************************************************/
void SwpProcessStateChange(void) {
    bool stateChanged;
    
    // The state machine should process until no further state changes occur
    do {
        // Determine the next state from the current state
        SwpState_t nextState = NextState(currentState);  
        stateChanged = (nextState != currentState);

        // Reset the current status flags before moving to the next state
        currentStatus.value = 0x0;

        if (stateChanged) {
            //printf("[DEBUG] Transitioning from state %d to state %d\n", currentState, nextState);

            // Exit the current state (if applicable)
            if (STATE_FUNCS[currentState].onExit != NULL) {
                STATE_FUNCS[currentState].onExit();
            }

            // Move to the new state
            currentState = nextState;

            // Reset the completion flag for the new state
            STATE_FUNCS[currentState].isComplete = false;

            // Enter the new state
            if (STATE_FUNCS[currentState].onEntry != NULL) {
                STATE_FUNCS[currentState].onEntry();
            }
        }

        // If the new state is complete, proceed to the next state
        if (STATE_FUNCS[currentState].isComplete) {
            printf("[DEBUG] State %d is complete. Proceeding to next state...\n", currentState);
        }

    } while (stateChanged && STATE_FUNCS[currentState].isComplete);

    // Handle the callback execution when we reach the waiting state
    if (currentState == SWP_WAITING && ctx.callback) {
        printf("[DEBUG] Executing callback as we have reached SWP_WAITING.\n");
        ctx.callback();  // Execute the callback function
        ctx.callback = NULL;  // Clear the callback to avoid re-execution
    }
}

/******************************************************************************/
/* Helper Functions for State Transitioning */
/******************************************************************************/

static SwpState_t NextState(SwpState_t state) {
    switch (state) {
        case SWP_INITIAL_ANALYSIS:
            if (currentStatus.isPeakFound) {
                return SWP_PEAK_CENTERING;  // Now goes to SWP_PEAK_CENTERING
            }
            if (currentStatus.isUndecided) {
                return SWP_UNDECIDED_TREND_CASE;
            }
            return SWP_SEGMENT_ANALYSIS;

        case SWP_SEGMENT_ANALYSIS:
            if (currentStatus.isUndecided) {
                return SWP_UNDECIDED_TREND_CASE;
            }
            if (currentStatus.isPeakFound) {
                return SWP_PEAK_CENTERING;  // Now goes to SWP_PEAK_CENTERING
            }
            return SWP_UPDATE_BUFFER_DIRECTION;

        case SWP_PEAK_CENTERING:
            if (currentStatus.isCentered) {
                return SWP_PEAK_FINDING_ANALYSIS;
            } else {
                return SWP_PEAK_CENTERING;  // Remain in SWP_PEAK_CENTERING until centered
            }

        case SWP_UPDATE_BUFFER_DIRECTION:
            return SWP_SEGMENT_ANALYSIS;

        case SWP_UNDECIDED_TREND_CASE:
            return SWP_SEGMENT_ANALYSIS;

        case SWP_PEAK_FINDING_ANALYSIS:
            if (currentStatus.isSweepDone) {
                return SWP_WAITING;
            }
            return SWP_PEAK_FINDING_ANALYSIS;

        case SWP_WAITING:
            if (currentStatus.isSweepRequested) {
                return SWP_INITIAL_ANALYSIS;
            }
            return SWP_WAITING;

        default:
            return SWP_WAITING;
    }
}
