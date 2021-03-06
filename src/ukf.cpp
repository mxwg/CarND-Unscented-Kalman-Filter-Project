#include "ukf.h"
#include "Eigen/Dense"
#include <iostream>

using namespace std;
using Eigen::MatrixXd;
using Eigen::VectorXd;
using std::vector;


/**
 * Initializes Unscented Kalman filter
 */
UKF::UKF() {
    // if this is false, laser measurements will be ignored (except during init)
    use_laser_ = true;

    // if this is false, radar measurements will be ignored (except during init)
    use_radar_ = true;

    // initial state vector
    x_ = VectorXd(5);

    // initial covariance matrix
    P_ = MatrixXd(5, 5);

    // laser state transition matrix
    H_laser_ = MatrixXd(2, 5);

    // laser measurement covariance matrix
    R_laser_ = MatrixXd(2, 2);

    // identity matrix for laser measurements
    I_ = MatrixXd(5, 5);

    // Process noise standard deviation longitudinal acceleration in m/s^2
    std_a_ = 2;

    // Process noise standard deviation yaw acceleration in rad/s^2
    std_yawdd_ = 2;

    // Laser measurement noise standard deviation position1 in m
    std_laspx_ = 0.15;

    // Laser measurement noise standard deviation position2 in m
    std_laspy_ = 0.15;

    // Radar measurement noise standard deviation radius in m
    std_radr_ = 0.3;

    // Radar measurement noise standard deviation angle in rad
    std_radphi_ = 0.03;

    // Radar measurement noise standard deviation radius change in m/s
    std_radrd_ = 0.3;

    // State dimension
    n_x_ = 5;

    // Augmented state dimension
    n_aug_ = 7;

    // Sigma point spreading parameter
    lambda_ = 3 - n_aug_;

    // Number of augmented sigma points
    n_sig_ = 2 * n_aug_ + 1;

    // Matrix of predicted sigma points
    Xsig_pred_ = MatrixXd(n_x_, n_sig_);

    // Weights for calculating new mean from sigma points
    weights_ = createWeights();
}

UKF::~UKF() {}

/**
 * @param {MeasurementPackage} meas_package The latest measurement data of
 * either radar or laser.
 */
void UKF::ProcessMeasurement(MeasurementPackage meas_package) {
    if (!this->is_initialized_) {
        initialize(meas_package);
        return; // early
    }

    // calculate delta t
    double delta_t = (meas_package.timestamp_ - previous_timestamp_) / 1000000.0; // microseconds to seconds
    previous_timestamp_ = meas_package.timestamp_;

    // predict, ...
    Prediction(delta_t);

    // then update!
    if (MeasurementPackage::LASER == meas_package.sensor_type_ && use_laser_) {
        UpdateLidar(meas_package);
    } else if (MeasurementPackage::RADAR == meas_package.sensor_type_ && use_radar_) {
        UpdateRadar(meas_package);
    }
}

/**
 * Predicts sigma points, the state, and the state covariance matrix.
 * @param {double} delta_t the change in time (in seconds) between the last
 * measurement and this one.
 */
void UKF::Prediction(double delta_t) {
    // The prediction code is taken from the lecture quizzes
    // Calculate augmented sigma points
    MatrixXd Xsig_aug = createAugmentedSigmaPoints();

    // predict sigma points
    predictSigmaPoints(Xsig_aug, delta_t);

    // predict new mean state
    x_.setZero();
    for (int i = 0; i < n_sig_; i++) {
        x_ = x_ + weights_(i) * Xsig_pred_.col(i);
    }

    // predict new state covariance matrix
    P_.fill(0.0);
    for (int i = 0; i < n_sig_; i++) {
        // state difference
        VectorXd x_diff = Xsig_pred_.col(i) - x_;
        x_diff(3) = normalize(x_diff(3));

        P_ = P_ + weights_(i) * x_diff * x_diff.transpose();
    }
}

/**
 * Updates the state and the state covariance matrix using a laser measurement.
 * @param {MeasurementPackage} meas_package
 */
void UKF::UpdateLidar(MeasurementPackage meas_package) {
    // Standard linear laser measurement update
    Eigen::VectorXd z_pred = H_laser_ * x_;
    Eigen::VectorXd z = meas_package.raw_measurements_;
    Eigen::VectorXd y = z - z_pred;

    Eigen::MatrixXd Ht = H_laser_.transpose();
    Eigen::MatrixXd S = H_laser_ * P_ * Ht + R_laser_;
    Eigen::MatrixXd Si = S.inverse();
    Eigen::MatrixXd PHt = P_ * Ht;
    Eigen::MatrixXd K = PHt * Si;
    x_ = x_ + (K * y);
    P_ = (I_ - K * H_laser_) * P_;

    printState("UpdateLidar");
}

/**
 * Updates the state and the state covariance matrix using a radar measurement.
 * @param {MeasurementPackage} meas_package
 */
void UKF::UpdateRadar(MeasurementPackage meas_package) {
    // The radar update code is taken from the lecture quizzes
    int n_z = 3;
    MatrixXd Zsig = sigmaPointsInMeasurementSpace(n_z);

    // mean predicted measurement
    VectorXd z_pred = VectorXd(n_z);
    z_pred.fill(0.0);
    for (int i = 0; i < n_sig_; i++) {
        z_pred = z_pred + weights_(i) * Zsig.col(i);
    }

    // measurement covariance matrix S
    MatrixXd S = MatrixXd(n_z, n_z);
    S.fill(0.0);
    for (int i = 0; i < n_sig_; i++) {
        // residual
        VectorXd z_diff = Zsig.col(i) - z_pred;
        z_diff(1) = normalize(z_diff(1));

        S = S + weights_(i) * z_diff * z_diff.transpose();
    }

    // add measurement noise covariance matrix
    MatrixXd R = MatrixXd(n_z, n_z);
    R << std_radr_ * std_radr_, 0, 0,
            0, std_radphi_ * std_radphi_, 0,
            0, 0, std_radrd_ * std_radrd_;
    S = S + R;


    VectorXd z = meas_package.raw_measurements_; // length 3


    MatrixXd Tc = MatrixXd(n_x_, n_z);

    // calculate cross correlation matrix
    Tc.fill(0.0);
    for (int i = 0; i < n_sig_; i++) {

        // residual
        VectorXd z_diff = Zsig.col(i) - z_pred;
        z_diff(1) = normalize(z_diff(1));

        // state difference
        VectorXd x_diff = Xsig_pred_.col(i) - x_;
        x_diff(3) = normalize(x_diff(3));

        Tc = Tc + weights_(i) * x_diff * z_diff.transpose();
    }

    // Kalman gain K;
    MatrixXd K = Tc * S.inverse();

    // residual
    VectorXd y = z - z_pred;
    y(1) = normalize(y(1));

    // update state mean and covariance matrix
    x_ = x_ + K * y;
    P_ = P_ - K * S * K.transpose();
    printState("UpdateRadar");
}

void UKF::initialize(MeasurementPackage measurement_pack) {
    // initial covariance
    P_ <<
       1, 0, 0, 0, 0,
            0, 1, 0, 0, 0,
            0, 0, 1, 0, 0,
            0, 0, 0, 1, 0,
            0, 0, 0, 0, 1;

    H_laser_ << 1, 0, 0, 0, 0,
            0, 1, 0, 0, 0;

    R_laser_ << 0.0225, 0,
            0, 0.0225;

    I_.setIdentity();

    cout << "EKF initialized with ";

    // initialize state according to measurement type
    if (measurement_pack.sensor_type_ == MeasurementPackage::RADAR) {
        // Convert radar from polar to cartesian coordinates and initialize state
        double rho = measurement_pack.raw_measurements_(0);
        double phi = measurement_pack.raw_measurements_(1);
        double rhod = measurement_pack.raw_measurements_(2);
        x_(0) = rho * cos(phi);
        x_(1) = rho * sin(phi);
        x_(2) = rhod;
        cout << " radar:" << endl;
    } else if (measurement_pack.sensor_type_ == MeasurementPackage::LASER) {
        // Initialize state
        x_(0) = measurement_pack.raw_measurements_(0);
        x_(1) = measurement_pack.raw_measurements_(1);
        cout << " laser:" << endl;
    }

    // initialize timestamp
    previous_timestamp_ = measurement_pack.timestamp_;

    printState("Initialization");

    // done initializing, no need to predict or update
    is_initialized_ = true;
}

void UKF::printState(std::string title) {
    std::cout << title << std::endl;
    std::cout << "x: " << x_.transpose() << std::endl;
    std::cout << "P:" << std::endl;
    std::cout << P_ << std::endl;
}

VectorXd UKF::createWeights() const {
    VectorXd weights_ = VectorXd(n_sig_);

    // set weights
    double weight_0 = lambda_ / (lambda_ + n_aug_);
    weights_(0) = weight_0;
    for (int i = 1; i < n_sig_; i++) {
        double weight = 0.5 / (n_aug_ + lambda_);
        weights_(i) = weight;
    }

    return weights_;
}

double UKF::normalize(double angle_rad) const {
    while (angle_rad > M_PI) angle_rad -= 2. * M_PI;
    while (angle_rad < -M_PI) angle_rad += 2. * M_PI;

    return angle_rad;
}

MatrixXd UKF::createAugmentedSigmaPoints() const {
    VectorXd x_aug = VectorXd(n_aug_);
    MatrixXd P_aug = MatrixXd(n_aug_, n_aug_);
    MatrixXd Xsig_aug = MatrixXd(n_aug_, n_sig_);

    // create augmented mean state
    x_aug.head(n_x_) = x_;
    x_aug(n_x_) = 0;
    x_aug(n_x_ + 1) = 0;

    // create augmented covariance matrix
    P_aug.setZero();
    P_aug.topLeftCorner(5, 5) = P_;
    P_aug(5, 5) = std_a_ * std_a_;
    P_aug(6, 6) = std_yawdd_ * std_yawdd_;

    // create square root matrix
    MatrixXd L = P_aug.llt().matrixL();

    // create augmented sigma points
    Xsig_aug.col(0) = x_aug;
    for (int i = 0; i < n_aug_; i++) {
        Xsig_aug.col(i + 1) = x_aug + sqrt(lambda_ + n_aug_) * L.col(i);
        Xsig_aug.col(i + 1 + n_aug_) = x_aug - sqrt(lambda_ + n_aug_) * L.col(i);
    }

    return Xsig_aug;
}

void UKF::predictSigmaPoints(const MatrixXd &Xsig_aug, double delta_t) {
    for (int i = 0; i < n_sig_; i++) {
        // extract values for better readability
        double p_x = Xsig_aug(0, i);
        double p_y = Xsig_aug(1, i);
        double v = Xsig_aug(2, i);
        double yaw = Xsig_aug(3, i);
        double yawd = Xsig_aug(4, i);
        double nu_a = Xsig_aug(5, i);
        double nu_yawdd = Xsig_aug(6, i);

        // predicted state values
        double px_p, py_p;

        // avoid division by zero
        if (fabs(yawd) > 0.001) {
            px_p = p_x + v / yawd * (sin(yaw + yawd * delta_t) - sin(yaw));
            py_p = p_y + v / yawd * (cos(yaw) - cos(yaw + yawd * delta_t));
        } else {
            px_p = p_x + v * delta_t * cos(yaw);
            py_p = p_y + v * delta_t * sin(yaw);
        }

        double v_p = v;
        double yaw_p = yaw + yawd * delta_t;
        double yawd_p = yawd;

        // add noise
        px_p = px_p + 0.5 * nu_a * delta_t * delta_t * cos(yaw);
        py_p = py_p + 0.5 * nu_a * delta_t * delta_t * sin(yaw);
        v_p = v_p + nu_a * delta_t;

        yaw_p = yaw_p + 0.5 * nu_yawdd * delta_t * delta_t;
        yawd_p = yawd_p + nu_yawdd * delta_t;

        // write predicted sigma point into right column
        Xsig_pred_(0, i) = px_p;
        Xsig_pred_(1, i) = py_p;
        Xsig_pred_(2, i) = v_p;
        Xsig_pred_(3, i) = yaw_p;
        Xsig_pred_(4, i) = yawd_p;
    }
}

MatrixXd UKF::sigmaPointsInMeasurementSpace(int n_z) {
    MatrixXd Zsig = MatrixXd(n_z, n_sig_);

    for (int i = 0; i < n_sig_; i++) {
        // extract values for better readability
        double p_x = Xsig_pred_(0, i);
        double p_y = Xsig_pred_(1, i);
        double v = Xsig_pred_(2, i);
        double yaw = Xsig_pred_(3, i);

        double v1 = cos(yaw) * v;
        double v2 = sin(yaw) * v;

        // radar measurement model
        Zsig(0, i) = sqrt(p_x * p_x + p_y * p_y);
        Zsig(1, i) = atan2(p_y, p_x);
        Zsig(2, i) = (p_x * v1 + p_y * v2) / sqrt(p_x * p_x + p_y * p_y);
    }
    return Zsig;
}
