#include "numcmc/mcmc.h"

namespace nu {
Mcmc::Mcmc(int K, int N, std::vector<std::vector<double>> init_sample) {
    std::vector<Walker> walkers;
    for (int k_walk = 0; k_walk < K; k_walk++) {
        Walker single_walker(N, init_sample[k_walk]);
        walkers.push_back(single_walker);
    };
    Ensemble create_sample(K, N, walkers);
    this->sample = create_sample;
    this->nwalkers = K;
    this->ndim = N;
}

Mcmc::Mcmc(int K, int N, char *file_name) {
    std::vector<std::vector<double>> init_sample = this->load_state(file_name);
    std::vector<Walker> walkers;
    for (int k_walk = 0; k_walk < K; k_walk++) {
        Walker single_walker(N, init_sample[k_walk]);
        walkers.push_back(single_walker);
    };
    Ensemble create_sample(K, N, walkers);
    this->sample = create_sample;
    this->nwalkers = K;
    this->ndim = N;
}

std::vector<std::vector<double>> Mcmc::get_chain() {
    std::vector<std::vector<double>> samples_chain;

    for (int k = 1; k <= this->nwalkers; k++) {
        Walker walker;
        walker = this->sample.getWalkerCopy(k);
        int steps = walker.getSteps();

        for (int i = 1; i <= steps; i++) {
            samples_chain.push_back(walker.getHistory()[i - 1]);
        };
    };

    return samples_chain;
}

std::vector<std::vector<double>> Mcmc::get_chain_walkers() {
    std::vector<std::vector<double>> samples_chain;

    for (int k = 1; k <= this->nwalkers; k++) {
        Walker walker;
        walker = this->sample.getWalkerCopy(k);
        int steps = walker.getSteps();

        for (int i = 1; i <= steps; i++) {
            std::vector<double> pos_aux = walker.getHistory()[i - 1];
            pos_aux.push_back(k);
            samples_chain.push_back(pos_aux);
        };
    };

    return samples_chain;
}

int Mcmc::save_state(char *file_name) {
    std::ofstream file;
    file.open(file_name);

    if (!(file.is_open())) {
        std::cout << "\n\n ****You should pass an opened file!****\n\n";
        return 0;
    };

    /*Writing samples*/

    for (int i = 1; i <= this->nwalkers; i++) {
        Walker aux_walker;
        aux_walker = this->sample.getWalkerCopy(i);
        for (int j = 0; j < this->ndim; j++) {
            file << aux_walker.getPos()[j];
            if (j != ndim - 1) file << ",";
        };
        if (i != nwalkers) file << std::endl;
    };
    file.close();
    return 1;
}

std::vector<std::vector<double>> Mcmc::load_state(char *file_name) {
    std::ifstream file;
    std::vector<std::vector<double>> ensemble_init;
    file.open(file_name);

    if (!(file.is_open())) {
        std::cout << "\n\n ****You should pass an opened file!****\n\n";
        return ensemble_init;
    };

    this->reset();

    /*Load samples*/
    do {
        std::string line, col;
        std::getline(file, line);

        // Create a stringstream from line
        std::stringstream ss(line);
        std::vector<double> walker_pos;
        // Extract each column name
        while (std::getline(ss, col, ',')) {
            double xx = std::stold(col);
            walker_pos.push_back(xx);
        };
        ensemble_init.push_back(walker_pos);
    } while (!file.eof());
    return ensemble_init;
}

int Mcmc::save_chain_walker(char *file_name, char *header) {
    std::ofstream sampling_file;
    sampling_file.open(file_name);

    std::vector<std::vector<double>> sampling_points;
    sampling_points = this->get_chain_walkers();

    sampling_file << header;

    /*Writing samples*/
    double nsteps = sampling_points.size();
    for (int i = 0; i < nsteps; i++) {
        for (int j = 0; j < ndim + 1; j++) {
            sampling_file << sampling_points[i][j];
            if (j != ndim) sampling_file << ",";
        };
        if (i != nsteps * nwalkers - 1) sampling_file << std::endl;
    };
    return 1;
}

int Mcmc::save_chain(char *file_name, char *header) {
    std::ofstream sampling_file;
    sampling_file.open(file_name);

    std::vector<std::vector<double>> sampling_points;
    sampling_points = this->get_chain();

    sampling_file << header;

    /*Writing samples*/
    double nsteps = sampling_points.size();
    for (int i = 0; i < nsteps; i++) {
        for (int j = 0; j < ndim; j++) {
            sampling_file << sampling_points[i][j];
            if (j != ndim - 1) sampling_file << ",";
        };
        if (i != nsteps * nwalkers - 1) sampling_file << std::endl;
    };
    return 1;
}

int Mcmc::run(std::function<double(std::vector<double> &)> func, int steps) {
    for (int i = 1; i <= steps; i++) {
        // 1:
        for (int k = 1; k <= nwalkers; k++) {
            // 0.0001 s
            Walker walker, walker_aux;
            double logPY, logPX, q, z, r;
            std::vector<double> Y;
            std::vector<double> Xj;
            std::vector<double> Xk;

            walker = sample.getWalker(1);
            // 2:
            // The loop constraint some region
            do {
                Xj.clear();
                Xk.clear();
                Y.clear();
                walker_aux = sample.getRandomWalkerCopy();
                // 3:
                double uniform_rand = double((rand() % 5001)) / 5001.;
                z = F(uniform_rand);
                // 4:
                for (int l = 0; l < this->ndim; l++) {
                    Xj.push_back(walker_aux.getPos()[l]);
                    Xk.push_back(walker.getPos()[l]);
                    Y.push_back(Xj[l] + z * (Xk[l] - Xj[l]));
                };
                // 5:
                logPY = func(Y);
            } while (logPY == -std::numeric_limits<double>::infinity());

            logPX = func(Xk);
            q = pow(z, ndim - 1) * exp(logPY - logPX);
            // 6:
            r = double((rand() % 5001)) / 5001.;
            // 7:"
            if (r <= q) {  // 8:
                walker.setPos(Y);
            }  // 9:
            else {
                // 10:
                walker.setPos(Xk);
            }
            sample.includeWalker(walker);
        };
    };

    return 0;
}

int Mcmc::run_parallel(std::function<double(std::vector<double> &)> func,
                       int steps, int threads) {
    std::vector<Ensemble> samples;
    samples = sample.divideEnsemble();

    for (int i = 1; i <= steps; i++) {
#pragma omp parallel
        {
#pragma omp for
            for (int ip = 0; ip <= 1; ip++) {
// 1:
#pragma omp parallel for num_threads(threads)
                for (int k = 1; k <= nwalkers / 2; k++) {
                    Walker walker, walker_aux;
                    double logPY, logPX, q, z, r;
                    std::vector<double> Y;
                    std::vector<double> Xj;
                    std::vector<double> Xk;

                    int ic = (ip - 1) * (-1);

                    walker = samples[ip].getWalker(1);

                    // 2:
                    // The loop constraint some region
                    do {
                        Xj.clear();
                        Xk.clear();
                        Y.clear();
                        walker_aux = samples[ic].getRandomWalkerCopy();
                        // 3:
                        double uniform_rand = double((rand() % 5001)) / 5001.;
                        z = F(uniform_rand);

                        // 4:
                        for (int l = 0; l < this->ndim; l++) {
                            Xj.push_back(walker_aux.getPos()[l]);
                            Xk.push_back(walker.getPos()[l]);
                            Y.push_back(Xj[l] + z * (Xk[l] - Xj[l]));
                        };
                        // 5:
                        logPY = func(Y);
                    } while (logPY == -std::numeric_limits<double>::infinity());

                    logPX = func(Xk);
                    q = pow(z, ndim - 1) * exp(logPY - logPX);
                    // 6:
                    r = double((rand() % 5001)) / 5001.;
                    // 7:"
                    if (r <= q) {  // 8:
                        walker.setPos(Y);
                    }  // 9:
                    else {
                        // 10:
                        walker.setPos(Xk);
                    }
                    samples[ip].includeWalker(walker);
                };
            };
        }
    };

    std::vector<Walker> aux_walkers1, aux_walkers2;
    aux_walkers1 = samples[0].getWalkers();
    aux_walkers2 = samples[1].getWalkers();
    aux_walkers1.insert(aux_walkers1.end(), aux_walkers2.begin(),
                        aux_walkers2.end());
    this->sample.setWalkers(aux_walkers1);
    return 0;
}

int Mcmc::run_notlog(std::function<double(std::vector<double> &)> func,
                     int steps) {
    for (int i = 1; i <= steps; i++) {
        // 1:
        for (int k = 1; k <= nwalkers; k++) {
            // 0.0001 s
            Walker walker, walker_aux;
            double logPY, logPX, q, z, r;
            std::vector<double> Y;
            std::vector<double> Xj;
            std::vector<double> Xk;

            walker = sample.getWalker(1);
            // 2:
            // The loop constraint some region
            do {
                Xj.clear();
                Xk.clear();
                Y.clear();
                walker_aux = sample.getRandomWalkerCopy();
                // 3:
                double uniform_rand = double((rand() % 5001)) / 5001.;
                z = F(uniform_rand);
                // 4:
                for (int l = 0; l < this->ndim; l++) {
                    Xj.push_back(walker_aux.getPos()[l]);
                    Xk.push_back(walker.getPos()[l]);
                    Y.push_back(Xj[l] + z * (Xk[l] - Xj[l]));
                };
                // 5:
                logPY = func(Y);
            } while (logPY == -std::numeric_limits<double>::infinity());

            logPX = func(Xk);
            q = pow(z, ndim - 1) * logPY / logPX;
            // 6:
            r = double((rand() % 5001)) / 5001.;
            // 7:"
            if (r <= q) {  // 8:
                walker.setPos(Y);
            }  // 9:
            else {
                // 10:
                walker.setPos(Xk);
            }
            sample.includeWalker(walker);
        };
    };

    return 0;
}

int Mcmc::reset() {
    this->sample.cleanEnsembleHistory();
    return 1;
}
}  // namespace nu
