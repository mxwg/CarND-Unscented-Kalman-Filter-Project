#include <iostream>
#include "tools.h"

using Eigen::VectorXd;
using Eigen::MatrixXd;
using std::vector;

Tools::Tools() {}

Tools::~Tools() {}

VectorXd Tools::CalculateRMSE(const vector<VectorXd> &estimations,
                              const vector<VectorXd> &ground_truth) {
    if (estimations.size() != ground_truth.size() || estimations.size() < 1) {
        throw std::runtime_error("Input sizes are incorrect!");
    }

    VectorXd rmse(4);
    rmse << 0, 0, 0, 0;

    //accumulate squared residuals
    for (int i = 0; i < estimations.size(); ++i) {
        // ... your code here
        for (size_t j = 0; j < 4; ++j) {
            double diff = estimations[i][j] - ground_truth[i][j];
            rmse[j] += (diff * diff);
        }
    }

    for (size_t j = 0; j < 4; ++j) {
        rmse[j] = rmse[j] / estimations.size();
    }
    for (size_t j = 0; j < 4; ++j) {
        rmse[j] = sqrt(rmse[j]);
    }

    return rmse;
}