#ifndef PERCEPTRON_H_
#define PERCEPTRON_H_


#include <vector>
#include <fstream>
using namespace std;

class Perceptron {
  public:
    Perceptron(int n_dims) : weights(n_dims, 0.0),
                             average_weights_(n_dims, 0.0),
                             last_update_(n_dims, 0) {
        int round_ = 0;
    }

    void next_round() {
        round_ += 1;
    }

    void update(int feature, int direction) {
        average_weights_[feature] += weights[feature] * (round_ - last_update_[feature]);
        last_update_[feature] = round_;
        weights[feature] += direction;
    }

    void finish() {
        for (int i = 0; i < weights.size(); ++i) {
            average_weights_[i] += weights[i] * (round_ - last_update_[i]);
            last_update_[i] = round_;
        }
    }

    // void update(const vector<int> &gold, const vector<int> &best) {
    //     for (int i = 0; i < gold.size(); ++i) {
    //         weights[gold[i]] += 1;
    //     }

    //     for (int i = 0; i < best.size(); ++i) {
    //         weights[best[i]] += 1;
    //     }
    // }

    void write(string file, bool average) {
        ofstream out;
        out.open(file.c_str());
        for (int i = 0; i < average_weights_.size(); ++i)
        {
            if (average) {
                out << average_weights_[i] << " ";
            } else {
                out << weights[i] << " ";
            }
        }
        out << endl;
        out.close();
    }

    vector<double> weights;

  private:
    int round_;
    vector<int> last_update_;
    vector<double> average_weights_;
};
#endif  // PERCEPTRON_H_
