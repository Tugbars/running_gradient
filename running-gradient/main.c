#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#include "windowed_running_gradient.h"
#include "running_cubic_gradient.h"
#include "running_peak_analysis.h"
#include "running_quadratic_gradient.h"
#include "rls_analysis_parameters.h"

void myCallbackFunction(void) {
    printf("Callback executed: State machine returned to SWP_WAITING.\n");
}

    MqsRawDataPoint_t test_buffer[BUFFER_SIZE];

int main() {
	//const double phaseAngles[] = { 11.26, 11.13, 11.276, 11.136, 11.194, 11.22, 11.18, 11.371, 11.269, 11.389, 11.377, 11.448, 11.248, 11.569, 11.442, 11.332, 11.525, 11.532, 11.427, 11.48, 11.431, 11.435, 11.515, 11.561, 11.489, 11.575, 11.693, 11.484, 11.619, 11.572, 11.708, 11.676, 11.528, 11.828, 11.598, 11.697, 11.823, 11.839, 11.889, 11.839, 11.927, 11.99, 11.826, 11.964, 11.813, 12.089, 12.076, 11.986, 12.192, 12.118, 11.997, 12.144, 12.311, 12.315, 12.293, 12.441, 12.35, 12.354, 12.399, 12.419, 12.455, 12.453, 12.566, 12.538, 12.725, 12.588, 12.89, 12.914, 12.913, 12.94, 13.023, 12.842, 13.141, 13.083, 13.321, 13.352, 13.372, 13.488, 13.555, 13.674, 13.66, 13.651, 13.997, 13.868, 13.969, 14.105, 14.191, 14.531, 14.278, 14.432, 14.523, 14.744, 14.441, 14.802, 15.066, 14.889, 15.088, 15.493, 15.494, 15.774, 16.081, 15.847, 15.77, 16.463, 16.188, 16.436, 16.388, 16.708, 16.842, 16.95, 17.413, 17.781, 17.929, 18.59, 18.478, 18.806, 18.738, 19.276, 19.744, 19.83, 20.157, 20.391, 20.367, 20.866, 21.19, 21.535, 21.994, 22.578, 22.869, 22.826, 23.654, 23.704, 24.724, 24.866, 25.609, 25.97, 26.53, 26.882, 28.051, 28.477, 29.039, 29.041, 30.509, 30.461, 31.413, 31.605, 32.46, 34.372, 34.084, 35.04, 35.014, 35.321, 36.64, 36.967, 37.323, 38.137, 39.721, 39.566, 40.457, 41.6, 42.555, 42.642, 43.222, 43.819, 43.611, 44.176, 44.248, 44.378, 44.385, 44.624, 44.413, 44.228, 43.884, 43.298, 43.342, 42.24, 41.29, 41.042, 40.263, 39.176, 38.162, 38.101, 36.628, 37.511, 35.735, 34.998, 33.44, 32.395, 31.241, 30.892, 29.863, 29.021, 28.65, 27.799, 26.718, 27.519, 25.865, 25.085, 23.494, 23.679, 22.736, 22.747, 22.706, 21.48, 21.577, 21.855, 20.304, 20.415, 19.763, 19.755, 19.377, 18.424, 18.938, 18.784, 18.147, 18.072, 17.786, 17.284, 17.527, 17.015, 16.833, 16.676, 16.693, 16.288, 16.071, 15.955, 15.876, 15.482, 15.065, 15.203, 15.227, 14.815, 15.08, 14.961, 14.7, 14.835, 14.526, 14.142, 14.214, 14.261, 14.06, 14.094, 13.824, 13.916, 13.689, 13.736, 13.817, 13.489, 13.69, 13.645, 13.332, 13.344, 13.16, 13.13, 13.111, 12.94, 12.997, 12.829, 12.906, 12.569, 12.824, 12.533, 12.456, 12.47, 12.578, 12.363, 12.328, 12.399, 12.285, 12.3, 12.28, 12.232, 12.366, 12.303, 12.174, 12.035, 11.957, 12.123, 11.931, 12.031, 11.943, 12.024, 11.989, 11.93, 11.702, 11.943, 11.827, 11.818, 11.877, 11.696, 11.784, 11.726, 11.617, 11.542, 11.503, 11.58, 11.639, 11.688, 11.514, 11.541, 11.267, 11.388, 11.45, 11.537, 11.489, 11.32, 11.366, 11.376, 11.269, 11.24, 11.417, 11.314, 11.296, 11.306, 11.231, 11.381, 11.173, 11.321, 11.21, 11.185, 11.298, 11.121, 11.287, 11.227, 11.112, 11.199, 11.208, 11.224, 11.21, 11.168, 11.168, 11.266, 11.075, 11.15, 10.992, 11.005, 11.081, 10.916, 10.984, 11.074, 10.954, 11.052, 11.105, 10.999, 10.953, 11.02, 10.945, 11.056, 11.065, 10.913, 10.958, 11.022, 11.038, 10.969, 10.887, 10.904, 10.936, 10.899, 10.988, 10.752 };
	
	const double phaseAngles[] = {10.544653, 10.688583, 10.666841, 10.662732, 10.535033, 10.612065, 10.577628, 10.524487, 10.511290, 10.520899, 10.605484, 10.506456, 10.693456, 10.667562, 10.640863, 10.553473, 10.684760, 10.752397, 10.671068, 10.667091, 10.641893, 10.625706, 10.701795, 10.607544, 10.689169, 10.695256, 10.717050, 10.677475, 10.691141, 10.730298, 10.732664, 10.710082, 10.713123, 10.759815, 10.696599, 10.663845, 10.716597, 10.780855, 10.795759, 10.802620, 10.720496, 10.753401, 10.709436, 10.746909, 10.737377, 10.754609, 10.765248, 10.692602, 10.837926, 10.755324, 10.756213, 10.843190, 10.862529, 10.751269, 10.902390, 10.817731, 10.859796, 10.887362, 10.835401, 10.824412, 10.860767, 10.819504, 10.907496, 10.831528, 10.821727, 10.830010, 10.915317, 10.858694, 10.921139, 10.927524, 10.894352, 10.889785, 10.956356, 10.938758, 11.093567, 10.844841, 11.094493, 11.035941, 10.982765, 11.071057, 10.996308, 11.099276, 11.142057, 11.137176, 11.157537, 11.007247, 11.144075, 11.183029, 11.172096, 11.164571, 11.192833, 11.227109, 11.141589, 11.311490, 11.239783, 11.295933, 11.199566, 11.232262, 11.333208, 11.337874, 11.322334, 11.288216, 11.280459, 11.247973, 11.288277, 11.415095, 11.297583, 11.360763, 11.288338, 11.434631, 11.456051, 11.578981, 11.419166, 11.478404, 11.660141, 11.544303, 11.652028, 11.638368, 11.651792, 11.621518, 11.763853, 11.760687, 11.771138, 11.678104, 11.783163, 11.932094, 11.948678, 11.962627, 11.937934, 12.077570, 11.981595, 12.096366, 12.032683, 12.094221, 11.979764, 12.217793, 12.235930, 12.129859, 12.411867, 12.396301, 12.413920, 12.445867, 12.480462, 12.470674, 12.537774, 12.562252, 12.810248, 12.733546, 12.861890, 12.918012, 13.033087, 13.245610, 13.184196, 13.414342, 13.611838, 13.626345, 13.715446, 13.851129, 14.113374, 14.588537, 14.653982, 15.250756, 15.618371, 16.459558, 18.144264, 23.523062, 40.229511, 38.351265, 38.085281, 37.500885, 37.153946, 36.893066, 36.705956, 36.559536, 35.938847, 36.391586, 36.194046, 36.391586, 36.119102, 35.560543, 35.599018, 34.958851, 35.393860, 34.904797, 35.401318, 34.863518, 34.046680, 34.508522, 34.043182, 34.704235, 33.556644, 33.888481, 33.533638, 33.452129, 32.930935, 32.669731, 32.772537, 32.805634, 32.246761, 32.075809, 31.864927, 31.878294, 32.241131, 31.965626, 31.553604, 30.843288, 30.784569, 31.436094, 31.170496, 30.552132, 30.500242, 30.167421, 29.911989, 29.586046, 29.478958, 29.718994, 29.611095, 29.557945, 28.463432, 29.341291, 28.821512, 28.447210, 27.861872, 27.855633, 27.910660, 28.425800, 27.715517, 27.617193, 27.093372, 26.968832, 26.977205, 27.170172, 26.251677, 26.633236, 26.224941, 25.874708, 25.593761, 26.392395, 24.904768, 25.331600, 24.530737, 25.074808, 25.310865, 24.337013, 24.442986, 24.500193, 24.130409, 24.062714, 24.064592, 23.533037, 23.977909, 22.924667, 22.806379, 23.130791, 22.527645, 22.570505, 22.932512, 22.486126, 22.594856, 22.383926, 22.115181, 22.105082, 21.151754, 21.074114, 21.240192, 20.977468, 20.771507, 21.184586, 20.495111, 20.650751, 20.656075, 20.433039, 20.005697, 20.216360, 19.982117, 19.703951, 19.572884, 19.332155, 19.544645, 18.666328, 19.219872, 18.934229, 19.186989, 18.694986, 18.096903, 18.298306, 17.704309, 18.023785, 18.224157, 18.182484, 17.642824, 17.739542, 17.474176, 17.270575, 17.604120, 17.631210, 16.639175, 17.107626, 17.024216, 16.852285, 16.780111, 16.838861, 16.539309, 16.092861, 16.131529, 16.221350, 16.087164, 15.821659, 15.695448, 15.693087, 16.047991, 15.682863, 15.724131, 15.263708, 15.638486, 15.443835, 15.602257, 15.122874, 14.918172, 14.968882, 14.843689, 14.861169, 15.052527, 15.056897, 14.690192, 14.686479, 14.567565, 14.365212, 14.253309, 14.289158, 14.227124, 14.069589, 14.074703, 13.869432, 13.861959, 13.782178, 13.882711, 13.908362, 13.727641, 13.600214, 13.594969, 13.535290, 13.602018, 13.502626, 13.579159, 13.207825, 13.426789, 13.178141, 13.286413, 12.958746, 13.189507, 13.079733, 13.138372, 12.986096, 12.854589, 12.858962, 12.903029, 12.852099, 12.644394, 12.558786, 12.636994};
    //test5August_2 = [10.979223, 10.977920, 11.117862, 11.029127, 11.018932, 11.112049, 11.063987, 11.170511, 11.134503, 11.027050, 11.090155, 11.205723, 11.218451, 11.317812, 11.240696, 11.297317, 11.209357, 11.283273, 11.209501, 11.222271, 11.294257, 11.352165, 11.385016, 11.266213, 11.355242, 11.423784, 11.490499, 11.387779, 11.445207, 11.540172, 11.590783, 11.541268, 11.649580, 11.624307, 11.690697, 11.726274, 11.676068, 11.742362, 11.815248, 11.815248, 11.876165, 11.816499, 11.914444, 11.917177, 11.876592, 12.120811, 12.215640, 12.250919, 12.228274, 12.287125, 12.297300, 12.287054, 12.441885, 12.487884, 12.508768, 12.541326, 12.544608, 12.864800, 12.859945, 12.886235, 12.971893, 13.172553, 13.218493, 13.228001, 13.410561, 13.554869, 13.927762, 14.036242, 14.012826, 14.025476, 14.734611, 14.726756, 15.057339, 15.139411, 16.040812, 16.479399, 17.713818, 19.410034, 24.969717, 35.463646, 35.537678, 35.403614, 35.469681, 35.317131, 35.298008, 35.257282, 35.288387, 35.183487, 35.047283, 34.995152, 34.734856, 35.009228, 34.687775, 34.582630, 34.406910, 34.359379, 34.011074, 33.692841, 33.639698, 33.482403, 33.221436, 33.269836, 33.510445, 33.135963, 32.735222, 32.568001, 32.737701, 32.323933, 32.060680, 32.119442, 31.737291, 31.799446, 31.469273, 30.892544, 31.279222, 30.687691, 30.462284, 30.485657, 29.939999, 29.645365, 30.169846, 29.423336, 28.923670, 29.356571, 28.741953, 28.686144, 28.326759, 28.023563, 27.790836, 27.836771, 27.314100, 27.243483, 26.915947, 26.900658, 26.614590, 26.082163, 25.839071, 26.208933, 26.040007, 25.525953, 25.524147, 25.019604, 25.331362, 24.785259, 24.476313, 24.472597, 23.968166, 23.653109, 23.594231, 22.973392, 23.278109, 22.672255, 22.544035, 22.531382, 22.331047, 22.259781, 21.894737, 22.079660, 21.321957, 21.557842, 20.855448, 21.075111, 21.237841, 20.675995, 20.632689, 20.120665, 20.303848, 20.126234, 19.967005, 19.889376, 19.394733, 19.815092, 19.288486, 19.260920, 18.891298, 18.818075, 18.702181, 18.580667, 18.698664, 18.175066, 18.139879, 17.912615, 18.083445, 17.512405, 17.771719, 17.531021, 17.247925, 17.161154, 16.966854, 16.816122, 16.807178, 16.924803, 16.976154, 16.421207, 16.499002, 16.528862, 15.971985, 16.252008, 16.109510, 16.146242, 15.968963, 15.979271, 15.780751, 15.779634, 15.661145, 15.834071, 15.618397, 15.513812, 15.666034, 15.392133, 15.299003, 15.341523, 15.054385, 14.831288, 15.085377, 14.813046, 14.614686, 14.730702, 14.552499, 14.568325, 14.406591, 14.389461, 14.303265, 14.322719, 14.099542, 14.274109, 14.183070, 14.152795, 14.013523, 14.000959, 13.932707, 13.895621, 13.530278, 13.652857, 13.603933, 13.730695, 13.531404, 13.486109, 13.498939, 13.445791, 13.135691, 13.438188, 13.348389, 13.191856, 13.043575, 13.131701, 13.171075, 12.990030, 12.981641, 13.004532, 12.813705, 12.868507, 12.963441, 12.818621, 12.864848, 12.875001, 12.728168, 12.810537, 12.729118, 12.713572, 12.635060, 12.671034, 12.551735, 12.632514, 12.521195, 12.404672, 12.465552, 12.540709, 12.384567, 12.405470, 12.269866, 12.436987, 12.168406, 12.249508, 12.211508, 12.247842, 12.312964, 12.192352, 11.997868, 12.214117, 12.047089, 12.026310, 12.091992, 12.127889, 12.033442, 12.026480, 12.034227, 11.895766, 11.864389, 12.038473, 11.856215, 11.786341, 11.907907, 11.853717, 11.779496, 11.756284, 11.651981, 11.751902, 11.767889, 11.528368, 11.769483, 11.806577, 11.727275, 11.695949, 11.693642, 11.526228, 11.585077, 11.489136, 11.511891, 11.525106, 11.510126, 11.634756, 11.404969, 11.484659, 11.590220, 11.496562, 11.347791, 11.370731, 11.547200, 11.507333, 11.415930, 11.444540, 11.569152, 11.544858, 11.377908, 11.519958, 11.259042, 11.332579, 11.372212, 11.264635, 11.360838, 11.415566, 11.238256, 11.234443, 11.349486, 11.146450, 11.159431, 11.424602, 11.259216, 11.208861, 11.294948, 11.278002, 11.468853, 11.052851, 11.208776, 11.184476, 11.201353, 11.190137, 11.101993, 11.031355, 11.140203, 11.237129, 10.996803, 11.160957, 11.170036, 11.063917, 11.081130, 11.180137, 11.146686, 11.054589, 10.918217];
    
	size_t size = sizeof(phaseAngles) / sizeof(phaseAngles[0]);
	
	/*
	 for (int i = 0; i < size; i++) {
	 printf("%d. %f ", i, phaseAngles[i]);
	 if ((i + 1) % 10 == 0) {
	     printf("\n");
	 }
	}
	*/
	
	
	for(int i =0; i < size; i++)
	{
	    test_buffer[i].phaseAngle = phaseAngles[i];
	}

    startSlidingWindowAnalysis(phaseAngles, size, myCallbackFunction); //AKTIVE EDILMESI LAZIM. 
    
    /*
    int start_index = 140;
    
    double total_gradient_sum = compute_total_second_order_gradient(test_buffer, size, start_index, 0.5);
    //can warn that we are close to peak, but we need to move. 
    
    printf("Total sum of second-order gradients: %.6f\n\n", total_gradient_sum);
    
    GradientTrendResult quadraticTrends = track_gradient_trends_with_quadratic_regression(test_buffer, size, start_index, 30, 0.5);
    //left count lower than right count, whatever the difference is go that much left. 
    //left count higher than right count, go right. whatever that difference is go that much left. 
    
        // Call the function to find and verify the peak
    QuadraticPeakAnalysisResult peak_result = find_and_verify_quadratic_peak(test_buffer, size, 153, 0.5);

    if (peak_result.peak_found) {
        printf("Peak found at index %u\n\n", peak_result.peak_index);
    } else {
        printf("No peak found.\n\n");
    }
    */

    
    //globalleri ayarlayalım. 

    //MAX TREND, MIN/MAX DECREASE TREND EKLENMELI. PEAK CENTERED ICIN THRESHOLD LAZIM. (4 tane oldu bile). 
    //TRUNCATION ALERT VERMELI verifylar.
    
	return 0;
}

//center diye state daha olmalı. 
//State machine'i nasıl yapmalıyız onu anlamız lazım şimdi. 

//ardından logici nasıl encapsulate edeceğiz. 
