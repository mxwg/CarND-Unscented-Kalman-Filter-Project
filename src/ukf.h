#ifndef UKF_H
#define UKF_H

#include "measurement_package.h"
#include "Eigen/Dense"
#include <vector>
#include <string>
#include <fstream>

using Eigen::MatrixXd;
using Eigen::VectorXd;

class UKF {
public:

    ///* initially set to false, set to true in first call of ProcessMeasurement
    bool is_initialized_;

    ///* if this is false, laser measurements will be ignored (except for init)
    bool use_laser_;

    ///* if this is false, radar measurements will be ignored (except for init)
    bool use_radar_;

    ///* state vector: [pos1 pos2 vel_abs yaw_angle yaw_rate] in SI units and rad
    VectorXd x_;

    ///* state covariance matrix
    MatrixXd P_;

    ///* Laser state transition matrix
    MatrixXd H_laser_;

    ///* Laser measurement covariance matrix
    MatrixXd R_laser_;

    ///* Identity matrix for laser measurements
    MatrixXd I_;

    ///* predicted sigma points matrix
    MatrixXd Xsig_pred_;

    ///* time when the state is true, in us
    long long time_us_;

    ///* Process noise standard deviation longitudinal acceleration in m/s^2
    double std_a_;

    ///* Process noise standard deviation yaw acceleration in rad/s^2
    double std_yawdd_;

    ///* Laser measurement noise standard deviation position1 in m
    double std_laspx_;

    ///* Laser measurement noise standard deviation position2 in m
    double std_laspy_;

    ///* Radar measurement noise standard deviation radius in m
    double std_radr_;

    ///* Radar measurement noise standard deviation angle in rad
    double std_radphi_;

    ///* Radar measurement noise standard deviation radius change in m/s
    double std_radrd_;

    ///* Weights of sigma points
    VectorXd weights_;

    ///* State dimension
    int n_x_;

    ///* Augmented state dimension
    int n_aug_;

    ///* Number of sigma points
    int n_sig_;

    ///* Sigma point spreading parameter
    double lambda_;

    ///* previous timestamp
    long previous_timestamp_;

    /**
     * Constructor
     */
    UKF();

    /**
     * Destructor
     */
    virtual ~UKF();

    /**
     * ProcessMeasurement
     * @param meas_package The latest measurement data of either radar or laser
     */
    void ProcessMeasurement(MeasurementPackage meas_package);

    /**
     * Prediction Predicts sigma points, the state, and the state covariance
     * matrix
     * @param delta_t Time between k and k+1 in s
     */
    void Prediction(double delta_t);

    /**
     * Updates the state and the state covariance matrix using a laser measurement
     * @param meas_package The measurement at k+1
     */
    void UpdateLidar(MeasurementPackage meas_package);

    /**
     * Updates the state and the state covariance matrix using a radar measurement
     * @param meas_package The measurement at k+1
     */
    void UpdateRadar(MeasurementPackage meas_package);

    /**
     * Initializes state and covariance depending on the measurement
     * @param meas_package The first measurement package received
     */
    void initialize(MeasurementPackage meas_package);

    /**
     * Prints the current state of the UKF
     * @param title The title for the print statement
     */
    void printState(std::string title);

private:
    /**
     * Creates the weights vector
     * @return the weights
     */
    VectorXd createWeights() const;

    /**
     * Normalize angles
     * @param angle_rad the angle to be normalized
     * @return the normalized angle
     */
    double normalize(double angle_rad) const;

    /**
     * Create augmented sigma points
     * @return the sigma point matrix
     */
    MatrixXd createAugmentedSigmaPoints() const;

    /**
     * Sets the predicted sigma points
     * @param Xsig_aug the augmented sigma point matrix
     * @param delta_t the time diffence
     */
    void predictSigmaPoints(const MatrixXd & Xsig_aug, double delta_t);

    /**
     * Transforms sigma points into measurment space for radar
     * @param n_z dimension of the measurement
     * @return the sigma point matrix in radar measurement space
     */
    MatrixXd sigmaPointsInMeasurementSpace(int n_z);
};

#endif /* UKF_H */
