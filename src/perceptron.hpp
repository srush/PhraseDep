#ifndef PERCEPTRON_H_
#define PERCEPTRON_H_


#include <vector>
#include <fstream>
#include <cmath>
#include <stdio.h>
#include <iostream>
using namespace std;

class Perceptron {
  public:
    Perceptron(int n_dims) : weights(n_dims, 0.0),
                             // average_weights_(n_dims, 0.0),
                             unnormalized_averaged_subgradient(n_dims, 0.0),
                             g_square_over_time(n_dims, 0.0){
                             // last_update_(n_dims, 0)
        round_ = 1;
        lambda_ = 0.2;
    }
    void set_lambda(double lambda){
        lambda_ = lambda;
    }
    double get_lambda(){
        return lambda_;
    }

    void next_round() {
        round_ += 1;
    }

    void update(long feature, int direction) {
        // average_weights_[feature] += weights[feature] * (round_ - last_update_[feature]);
        // last_update_[feature] = round_;

        // lpk: using ada grad l1 here
        unnormalized_averaged_subgradient[feature] += direction;
        g_square_over_time[feature] += pow(direction, 2);
        // cerr << "direction " << direction << endl;
        // cerr << "unnormalized_averaged_subgradient " << unnormalized_averaged_subgradient[feature] << endl;
        // cerr << "round_ "<< round_ << endl;

        // this is l1

        double abs_normalized_averaged_subgradient =
                ((unnormalized_averaged_subgradient[feature])) / ((double)round_);
        abs_normalized_averaged_subgradient = abs_normalized_averaged_subgradient >= 0 ? abs_normalized_averaged_subgradient : (-abs_normalized_averaged_subgradient);
        if ( (abs_normalized_averaged_subgradient) <= lambda_){
            weights[feature] = 0;
        } else {
            weights[feature] = (1.0) * sgn(unnormalized_averaged_subgradient[feature]) * ( ((double)round_) / sqrt(g_square_over_time[feature]) ) * (abs_normalized_averaged_subgradient - lambda_);
        }

        // this is l2, just for convinient, use lambda as the hyper-para, but note the meaning is very different
        // TODO: DO NOT USE THIS, THIS IS BUGGY NOW. STILL FIXING.
        // weights[feature] = ( 1.0 / sqrt(g_square_over_time[feature]) ) * (unnormalized_averaged_subgradient[feature] / lambda_);


    }

    // void finish() {
    //     for (int i = 0; i < weights.size(); ++i) {
    //         average_weights_[i] += weights[i] * (round_ - last_update_[i]);
    //         last_update_[i] = round_;
    //     }
    // }

    template <typename T> int sgn(T val) {
        return (T(0) < val) - (val < T(0));
    }

    // void update(const vector<int> &gold, const vector<int> &best) {
    //     for (int i = 0; i < gold.size(); ++i) {
    //         weights[gold[i]] += 1;
    //     }

    //     for (int i = 0; i < best.size(); ++i) {
    //         weights[best[i]] += 1;
    //     }
    // }

    void write(string file) {
        ofstream out;
        out.open(file.c_str());
        for (int i = 0; i < weights.size(); ++i) {
            out << weights[i] << " ";
        }
        out << endl;
        out.close();
    }

    vector<double> weights;

  private:
    long round_;
    double lambda_;
    //vector<int> last_update_;
    vector<double> unnormalized_averaged_subgradient;
    vector<double> g_square_over_time;
    //vector<double> average_weights_;
};
#endif  // PERCEPTRON_H_
