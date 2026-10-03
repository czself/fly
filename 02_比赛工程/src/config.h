#ifndef F22_CONTEST_CONFIG_H
#define F22_CONTEST_CONFIG_H
/* Evidence gates: keep zero until the corresponding signed-axis experiments pass.
 * There is deliberately no motor-output enable option in this project. */
#define IMU_MOUNT_VERIFIED 0U
#define FLOW_CALIBRATED 0U
/* SENSOR -> BODY forward/right/down. Level stationary specific force must be -Z. */
static const float imu_mount[9] = {1,0,0, 0,1,0, 0,0,1};
static const FlowCalibration flow_calibration = {
    .radians_per_count = .002131946f,
    .translation = {1,0,0,1},
    .rotation = {0,0,0,0},
    .calibrated = FLOW_CALIBRATED
};
#endif
