//
// Created by Mario Nicolò De Matteis on 24/11/22.
//

#ifndef PORTFOLIO_PORTFOLIO_H
#define PORTFOLIO_PORTFOLIO_H

#include "Miner.h"

#include <algorithm>
#include <iostream>
#include <future>
#include <string>
#include <thread>
#include <vector>
#include <list>

#include <quote.hpp>
#include <json.hpp>

using namespace nlohmann;
using namespace std;

class Portfolio {
private:

    struct portfolio_result {
        double liquidity;
        double success_ratio;
        vector<double> betas;
        vector<double> amounts;
        vector<double> returns;
        vector<double> coefficients;
    };

    const struct asset_info {
        string name;
        string ticker;
    } ASSET;

    string start_date, end_date;
    Miner* miner;

    vector<string> assets;
    double liquidity;
    size_t n_assets, n_combs;
    string cur;

    vector<vector<double>> coefficients;
    map<string, double> last_prices;

    vector<future<double>> thread_simulation;
    map<vector<double>, vector<future<double>>*> simulation;
    map<vector<double>, portfolio_result> simulation_result;

    auto returns(int days,
                 const vector<double>& _coefficients) -> double;

public:

    Portfolio(const vector<string>& instruments,
              const string& starting_period,
              const string& ending_period,
              const string& tf,
              double cash,
              const string& currency = "€",
              size_t n_combinations = 10000,
              int m_size = 8);
    explicit Portfolio(const string& json_filename,
              int m_size = 8);
    ~Portfolio();

    auto generateCoefficients(size_t n_sample) -> void;
    auto simulate(int days,
                  int length) -> map<vector<double>, vector<future<double>>*>*;
    auto printOutcome() -> void;
    auto saveToCSV(const string& filename = "result.csv") -> void;
    auto saveReturns(const string& dir = "returns", const string& filename = "result.json") -> void;

    // Vectorized Version
    auto simulateVectorized(int days, int length);

};


#endif //PORTFOLIO_PORTFOLIO_H
