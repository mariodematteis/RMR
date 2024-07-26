//
// Created by Mario Nicolò De Matteis on 05/12/22.
//

#ifndef CPP_PORTFOLIO_MODELLING_MINER_H
#define CPP_PORTFOLIO_MODELLING_MINER_H


#include <iostream>
#include <iterator>
#include <fstream>
#include <future>
#include <random>
#include <string>
#include <list>
#include <map>
#include <set>

#include <quote.hpp>
#include <json.hpp>

using namespace std;
using namespace nlohmann;

class Miner {

private:
    struct sample_asset {
        time_t date_source;
        double open;
        double high;
        double low;
        double close;
        double returns;
    };

    map<string, vector<string>> assets_by_sector; // HashMap filtering list of assets through Sector
    map<string, vector<string>> assets_by_class; // HashMap filtering list of assets through Class
    map<string, map<string, sample_asset>> dataset;
    map<string, vector<double>> returns_dataset;

    string filename_json;
    string period;
    string timeframe;

    vector<string> assets_list; // Vector of assets contained in the file

    json financial_instruments;

    vector<Quote*> quotes;

    template<typename Iter, typename RandomGenerator> Iter uniform_return(Iter start,
            Iter end,
            RandomGenerator& g);
    map<string, double> last_returns;
    map<string, map<pair<double, double>, map<pair<double, double>, double>>> probabilities;
    map<string, map<pair<double, double>, int>> cardinality;
    map<string, vector<pair<double, double>>> blocks;


public:
    Miner(const string& starting_period,
          const string& ending_period,
          const string& tf,
          const string& source = "financial_instruments.json",
          int size = 10);
    ~Miner();

    auto print() -> void;
    auto getDataset() -> map<string, map<string, sample_asset>>;
    auto getFinancialInstruments() -> json *;

    auto getAssetName() -> vector<string> *;
    auto getAssetByClass(const string& asset_class) -> const vector<string>&;
    auto getAssetBySector(const string& sector) -> const vector<string>&;

    auto getReturns(const string& asset) -> vector<double>;
    auto getRandomReturn(const string& asset) -> double;
    auto ReturnsMarkovChain(const string& asset,
                            pair<double, double> p) -> pair<pair<double, double>, double>;
    auto getLastClosePrice(const string& asset) -> double;

};


#endif //CPP_PORTFOLIO_MODELLING_MINER_H
