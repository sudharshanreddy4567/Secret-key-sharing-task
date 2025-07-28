#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <random>
#include <stdexcept>
#include "sha256.h"              /
#include "json.hpp"              

using json = nlohmann::json;
using namespace std;
using Share = pair<int, int>;
vector<int> generateCoefficients(int secret, int k) {
    vector<int> coeffs = {secret};
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(1, 100);

    for (int i = 1; i < k; ++i) {
        coeffs.push_back(dis(gen));
    }
    return coeffs;
}
int evalPolynomial(const vector<int>& coeffs, int x) {
    int result = 0;
    int power = 1;
    for (int coeff : coeffs) {
        result += coeff * power;
        power *= x;
    }
    return result;
}
map<int, int> generateShares(int secret, int n, int k) {
    map<int, int> shares;
    vector<int> coeffs = generateCoefficients(secret, k);
    for (int i = 1; i <= n; ++i) {
        shares[i] = evalPolynomial(coeffs, i);
    }
    return shares;
}

int lagrangeInterpolation(const vector<Share>& shares) {
    int secret = 0;
    for (size_t i = 0; i < shares.size(); ++i) {
        int xi = shares[i].first;
        int yi = shares[i].second;

        double li = 1;
        for (size_t j = 0; j < shares.size(); ++j) {
            if (i != j) {
                int xj = shares[j].first;
                li *= double(0 - xj) / (xi - xj);
            }
        }
        secret += yi * li;
    }
    return round(secret);
}

void loadAndGenerateShares() {
    ifstream file("shares.json");
    json input;
    file >> input;

    int secret = input["secret"];
    int k = input["k"];
    int n = input["n"];

    auto shares = generateShares(secret, n, k);

    string hash = sha256(to_string(secret));
    cout << "Original Secret Hash: " << hash << endl;

    ofstream out("generated_shares.json");
    json output;
    for (const auto& [x, y] : shares) {
        output["shares"].push_back({{"x", x}, {"y", y}});
    }
    output["hash"] = hash;
    out << output.dump(4);
}

void evaluateShares() {
    ifstream file("eval_input.json");
    json input;
    file >> input;

    string trueHash = input["hash"];
    vector<Share> shares;

    for (const auto& item : input["shares"]) {
        shares.emplace_back(item["x"], item["y"]);
    }

    int reconstructed = lagrangeInterpolation(shares);
    string recHash = sha256(to_string(reconstructed));

    cout << "Reconstructed Secret: " << reconstructed << endl;

    if (recHash != trueHash) {
        throw runtime_error("❌ Invalid shares! Secret hash mismatch.");
    } else {
        cout << "Valid shares. Secret verified." << endl;
    }
}

