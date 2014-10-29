#ifndef PERCEPTRON_H_
#define PERCEPTRON_H_


#include <vector>
using namespace std;

class Perceptron {
  public:
    Perceptron(int n_dims) : weights(n_dims, 0.0) {}


    void update(int feature, int direction) {
        weights[feature] += direction;

    }

    void update(const vector<int> &gold, const vector<int> &best) {
        for (int i = 0; i < gold.size(); ++i) {
            weights[gold[i]] += 1;
        }

        for (int i = 0; i < best.size(); ++i) {
            weights[best[i]] += 1;
        }
    }

    vector<double> weights;
  private:

};
#endif  // PERCEPTRON_H_
