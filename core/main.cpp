#include <iostream>
#include "common/libp2p/soralog.hpp"
#include "proofs/proof_param_provider.hpp"

int main() {
  fc::libp2pSoralog();

  OUTCOME_EXCEPT(
      a, fc::proofs::ProofParamProvider::readJson("../../core/proofs/parameters.json"));
  using namespace std;
  using namespace std::chrono;

  auto start = steady_clock::now();
  OUTCOME_EXCEPT(fc::proofs::ProofParamProvider::getParams(a, 2048));
  auto finish = steady_clock::now();
  auto dur = finish - start;
  std::cerr << duration_cast<milliseconds>(dur).count() << endl;
}
