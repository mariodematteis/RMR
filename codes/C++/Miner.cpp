//
// Created by Mario Nicolò De Matteis on 05/12/22.
//

#include "Miner.h"

Miner::Miner(const string& starting_period,
             const string& ending_period,
             const string& tf,
             const string& source,
             int size) {

    std::ifstream json_financial_instruments_file(source, ifstream::in);
    financial_instruments = json::parse(json_financial_instruments_file);

    for (json::iterator it = financial_instruments.begin(); it != financial_instruments.end(); it++) {
        assets_list.push_back(it.key());
        assets_by_class[it.value()["ASSET_CLASS"]].push_back(it.key());
        assets_by_sector[it.value()["SECTOR"]].push_back(it.key());

        double minimum_return = 1e3;
        double maximum_return = -1e3;

        auto quote = new Quote(it.key());
        quotes.push_back(quote);
        quote->getHistoricalSpots(starting_period.c_str(), ending_period.c_str(), tf.c_str()); // Fetching data...
        for (int i = 0; i < quote->nbSpots(); i++) {
            auto spot = quote->getSpot((size_t)i);

            auto tmpReturn = ((spot.getClose() - spot.getOpen()) / spot.getOpen()) * 100;

            sample_asset _info = {};
            _info.date_source = spot.getDate(); // Date Information - Date Format DD-MM-YYYY
            _info.open = spot.getOpen(); // Open Price
            _info.high = spot.getHigh(); // High Price
            _info.low = spot.getLow(); // Low Price
            _info.close = spot.getClose(); // Close Price
            _info.returns = (spot.getClose() - spot.getOpen()) / spot.getOpen(); // Computing the returns
            returns_dataset[it.key()].push_back(((spot.getClose() - spot.getOpen()) / spot.getOpen()) * 100); // Saving the return percentages in the HashMap, adding the value to the vector indexed by the asset
            dataset[it.key()].insert(pair<string, sample_asset>(spot.getDateToString(), _info)); // Associating to each asset, a hashmap such that to each date, a sample_asset structure is assigned

            if (tmpReturn < minimum_return) {
                minimum_return = tmpReturn;
            }

            if (tmpReturn > maximum_return) {
                maximum_return = tmpReturn;
            }

        }

        auto delta = abs( minimum_return - maximum_return);
        auto difference = delta / size;
        auto start_ = minimum_return;

        for (int i = 0; i < size; i++) {
            blocks[it.key()].emplace_back(start_, start_ + difference);
            start_ += difference;
        }

        auto b = blocks[it.key()];

        for (int i = 1; i < returns_dataset[it.key()].size(); i++) {
            for (const auto &p1 : b) {
                for (const auto &p2 : b) {
                    probabilities[it.key()][make_pair(p2.first,
                                                      p2.second)][make_pair(p1.first,
                                                                            p1.second)] = 0;
                }
            }
        }

        for (int i = 1; i < returns_dataset[it.key()].size(); i++) {
            auto current_value = returns_dataset[it.key()].at(i);
            for (const auto &p1 : b) {
                if (current_value >= p1.first && current_value <= p1.second) {
                    auto previous_value = returns_dataset[it.key()].at(i - 1);
                    for (const auto &p2 : b) {
                        if (previous_value >= p2.first && previous_value <= p2.second) {
                            probabilities[it.key()][make_pair(p2.first,
                                                              p2.second)][make_pair(p1.first,
                                                                                    p1.second)] += 1;
                        }
                    }
                }
            }
        }

        for (const auto &p1 : b) {
            for (const auto &p2 : b) {
                auto s = probabilities[it.key()][make_pair(p1.first,
                                                 p1.second)][make_pair(p2.first,
                                                             p2.second)];
                if (cardinality[it.key()].contains(make_pair(p1.first,
                                                             p1.second))) {
                    cardinality[it.key()][make_pair(p1.first,
                                                    p1.second)] += (int) s;
                } else {
                    cardinality[it.key()][make_pair(p1.first,
                                                    p1.second)] = (int) s;
                }
            }
        }

        for (const auto &[k, v] : cardinality[it.key()]) {
            if (v == 0) {
                continue;
            }

            for (const auto &p2 : b) {
                probabilities[it.key()][make_pair(k.first,
                                                  k.second)][make_pair(p2.first,
                                                                       p2.second)] =
                                                                               probabilities[it.key()][make_pair(k.first,
                                                                                                                 k.second)][make_pair(p2.first,
                                                                                                                                      p2.second)] / v;
            }
        }
    }

    for (const auto &[k1, v1]: probabilities) {
        cout << "ASSET: " << k1 << endl;
        for (const auto &[k2, v2] : v1) {
            cout << "* ( " << k2.first << " %, " << k2.second << " % ) *" << endl << endl;
            for (const auto &[k3, v3] : v2) {
                cout << "( " << k3.first << " %, " << k3.second << " % ) -> " << v3 << endl;
            }
            cout << endl << endl;
        }
    }
    cout << endl << endl;

}

Miner::~Miner() {
    for (auto q : quotes) {
        delete q;
    }
}

auto Miner::getDataset() -> map<string, map<string, sample_asset>> {
    return dataset;
}

auto Miner::getFinancialInstruments() -> json* {
    return &financial_instruments;
}

auto Miner::getAssetName() -> vector<string> * {
    return &assets_list;
}

auto Miner::getAssetByClass(
    const string& asset_class
    ) -> const vector<string>& {
    return assets_by_class[asset_class];
}

auto Miner::getAssetBySector(
    const string& sector
    ) -> const vector<string>& {
    return assets_by_sector[sector];
}

auto Miner::print() -> void {
    for (const auto&[k, v]: dataset) {
        cout << "TICKER: " << k << endl << endl;
        for (const auto&[k1, v1] : v) {
            cout << "DATE: " << k1
                << " - RETURN: " << v1.returns
                << " - OPEN: " << v1.open
                << " - HIGH: " << v1.high
                << " - LOW: " << v1.low
                << " - CLOSE: " << v1.close << endl;
        }
        cout << endl << endl;
    }
}

auto Miner::getReturns(const string& asset) -> vector<double> {
    vector<double> result;
    auto asset_information = dataset[asset];

    for (const auto &[k, v]: asset_information) {
        result.push_back(v.returns);
    }
    return result;
}

auto Miner::getRandomReturn(const string& asset) -> double {
    auto info = dataset[asset];
    static std::random_device rd;
    static std::mt19937 gen(rd());
    auto choice = uniform_return(info.begin(), info.end(), gen);
    return choice->second.returns;
}

auto Miner::getLastClosePrice(const string& asset) -> double {
    auto last_element = dataset[asset].end();
    --last_element;
    return last_element->second.close;
}

template<typename Iter,
        typename RandomGenerator>
Iter Miner::uniform_return(Iter start, Iter end, RandomGenerator& g) {
    std::uniform_int_distribution<> dis(0, std::distance(start, end) - 1);
    std::advance(start, dis(g));
    return start;
}

auto Miner::ReturnsMarkovChain(const string& asset, pair<double, double> p) -> pair<pair<double, double>, double> {
    if (p.first == 0 && p.second == 0) {
        auto random_value = getRandomReturn(asset);
        for (const auto &p1: blocks[asset]) {
            if (random_value >= p1.first && random_value <= p1.second) {
                return make_pair(p1, random_value);
            }
        }
    }

    auto block = blocks[asset];
    auto raw_distribution = probabilities[asset][p];

    double sum = 0;

    vector<double> distribution;
    for (const auto &[k, v] : raw_distribution) {
        distribution.push_back(v);
        sum += v;
    }

    if (sum == 0) {
        auto random_value = getRandomReturn(asset);
        for (const auto &p1: blocks[asset]) {
            if (random_value >= p1.first && random_value <= p1.second) {
                return make_pair(p1, random_value);
            }
        }
    }

    std::random_device rd;
    std::mt19937 gen(rd());
    std::discrete_distribution<> d(distribution.begin(), distribution.end());
    auto r = block[d(gen)];

    auto random_ = getRandomReturn(asset);
    while (random_ < r.first || random_ > r.second) {
        random_ = getRandomReturn(asset);
    }

    return make_pair(r, random_);
}