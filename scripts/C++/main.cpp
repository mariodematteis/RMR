

#include "Portfolio.h"
#include "Miner.h"

int main() {

    auto portfolio = new Portfolio({"UCG.MI", "MONC.MI", "STLAM.MI"},
                                   "2022-01-01",
                                   "2023-04-13",
                                   "1d",
                                   10000,
                                   "€",
                                   150);

    portfolio->simulate(3, 10000);
    portfolio->printOutcome();
    portfolio->saveToCSV();
    portfolio->saveReturns();

    return 0;
}