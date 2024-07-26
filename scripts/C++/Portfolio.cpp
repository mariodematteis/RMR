//
// Created by Mario Nicolò De Matteis on 24/11/22.
//

#include "Portfolio.h"

template<typename Numeric, typename Generator = std::mt19937>
Numeric random(Numeric from, Numeric to)
{
    thread_local static Generator gen(std::random_device{}());

    using dist_type = typename std::conditional<std::is_integral<Numeric>::value,
            std::uniform_int_distribution<Numeric>,
            std::uniform_real_distribution<Numeric>>::type;

    thread_local static dist_type dist;
    return dist(gen, typename dist_type::param_type{from, to});
}

Portfolio::Portfolio(const vector<string>& instruments,
                     const string& starting_period,
                     const string& ending_period,
                     const string& tf,
                     double cash,
                     const string& currency,
                     size_t n_combinations,
                     int m_size) {
    start_date = starting_period;
    end_date = ending_period;
    miner = new Miner(starting_period,
                      ending_period,
                      tf,
                      "financial_instruments.json",
                      m_size);
    assets = instruments;
    n_assets = instruments.size();
    liquidity = cash;
    cur = currency;
    n_combs = n_combinations;
    generateCoefficients(n_combs);

    for (const auto &i : instruments) {
        last_prices[i] = miner->getLastClosePrice(i);
    }

}

Portfolio::Portfolio(const string& filename,
                     int m_size) {

    ifstream file_reader(filename);
    auto json_filename = json::parse(file_reader);

    miner = new Miner(json_filename["STARTING_TIME"],
                      json_filename["ENDING_TIME"],
                      json_filename["TIMEFRAME"],
                      "financial_instruments.json",
                      m_size);
    assets = json_filename["ASSETS"];
    n_assets = assets.size();
    liquidity = double(json_filename["CASH"]);
    cur = json_filename["CURRENCY"];
    n_combs = json_filename["N_COMBINATIONS"];
    generateCoefficients(n_combs);

    for (const auto &i : assets) {
        last_prices[i] = miner->getLastClosePrice(i);
    }

}

Portfolio::~Portfolio() {
    delete miner;
}

void Portfolio::generateCoefficients(size_t n_sample) {
    for (int i = 0; i < n_sample; i++) {
        vector<double> v;
        double sum = 0;
        for (int j = 0; j < n_assets - 1; j++) {
            auto coefficient = random(0.0, liquidity - sum);
            sum += coefficient;
            v.push_back(coefficient);
        }
        v.push_back(liquidity - sum);
        coefficients.push_back(v);
    }

}

auto Portfolio::returns(int days,
                        const vector<double>& _coefficients) -> double {
    double r = 0.0;

    for (int d = 0; d < days; d++) {
        for (int i = 0; i < n_assets; i++) {
            const auto& _asset = assets.at(i);
            r += ((miner->getRandomReturn(_asset)) * _coefficients.at(i));
        }
    }

    return r;
}

auto Portfolio::simulate(int days,
                         int length) -> map<vector<double>, vector<future<double>>*>* {

    for (const auto &v : coefficients) {
        vector<double> financial_instruments_betas;
        vector<double> financial_instruments_amounts;
        double liquidity_left = liquidity;

        for (int j = 0; j < n_assets; j++) {
            auto cardinality = floor(v.at(j) / last_prices[assets.at(j)]);
            double amount = cardinality * last_prices[assets.at(j)];

            financial_instruments_betas.push_back(cardinality);
            financial_instruments_amounts.push_back(amount);
            liquidity_left -= amount;
        }

        vector<double> samples;
        thread_simulation.clear();

        for (int i = 0; i < length; i++) {
            thread_simulation.push_back(async(launch::async,
                                              &Portfolio::returns,
                                              this,
                                              days,
                                              financial_instruments_betas));
        }

        portfolio_result p_result = {};
        double s_ratio = 0;
        for (auto & i : thread_simulation) {
            auto return_ = i.get();
            if (return_ > 0) s_ratio++;
            p_result.returns.push_back(return_);
        }

        vector<double> b_ = v;
        double l = liquidity;

        for_each(b_.begin(), b_.end(), [l](double &c){ c /= l; });

        p_result.coefficients = b_;
        p_result.liquidity = liquidity_left;
        p_result.betas = financial_instruments_betas;
        p_result.amounts = financial_instruments_amounts;
        p_result.success_ratio = s_ratio / double(thread_simulation.size());
        simulation_result[v] = p_result;
        simulation[v] = &thread_simulation;

    }

    return &simulation;
}

auto Portfolio::printOutcome() -> void {
    for (const auto &[k, v] : simulation_result) {
        cout << "{";
        for (int i = 0; i < v.amounts.size(); i++) {
            cout << " " << v.coefficients[i] << " ";
        }
        cout << "} -> RATIO: " << v.success_ratio << " - LIQUIDITY: " << v.liquidity << endl;
    }
}

auto Portfolio::saveToCSV(const string& filename) -> void {
    std::ofstream result;
    result.open (filename);
    result << "LIQUIDITY;SUCCESS_RATIO;BETAS;AMOUNTS;COEFFICIENTS;CAPITAL;ASSETS" << endl;
    for (const auto &[k, v] : simulation_result) {
        result << v.liquidity << ";" << v.success_ratio << ";[";
        for (int i = 0; i < v.betas.size() - 1; i++) {
            result << v.betas[i] << ", ";
        }
        result << v.betas[v.betas.size() - 1] << "];[";
        for (int i = 0; i < v.amounts.size() - 1; i++) {
            result << v.amounts[i] << ", ";
        }
        result << v.amounts[v.amounts.size() - 1] << "];[";
        for (int i = 0; i < v.coefficients.size() - 1; i++) {
            result << v.coefficients[i] << ", ";
        }
        result << v.coefficients[v.coefficients.size() - 1] << "];" << liquidity << ";[";
        for (int i = 0; i < assets.size() - 1; i++) {
            result << assets[i] << ", ";
        }
        result << assets[assets.size() - 1] << "]" << endl;
    }
    result.close();
}

auto Portfolio::saveReturns(const string& dir, const string& filename) -> void {
    json j_result;

    int i = 0;
    std::ofstream result;
    result.open(dir + "/" + filename);

    for (const auto &[k, v] : simulation_result) {
        j_result[i] = {v.coefficients, v.returns};
        i++;
    }

    result << j_result;
    result.close();

}