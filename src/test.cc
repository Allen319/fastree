#include <iostream>
#include <numeric>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>
#include <random>

// Include ROOT headers
#include "TTree.h"
#include "TFile.h"

// Include User-defined headers
#include "ExceptionV.h"
#include "Logger.h"
#include "Options.h"
std::pair<float, float> calculateBaselineAndStdDev(const std::vector<float> &SiPMWave, int readoutWindow, float thres) {
    int count = 0;
    float baseline = 0;
    float variance = 0;
    std::vector<int> signal_start, signal_end;

    // Identify the start and end indices of regions where the signal is above the threshold
    for (int i = 0; i < readoutWindow; i++) {
        if (SiPMWave[i] > thres) {
            int start = i;
            int end = i;

            for (; start >= 0 && SiPMWave[start] > thres; --start);
            for (; end < readoutWindow && SiPMWave[end] > thres; ++end);

            signal_start.push_back(start + 1);
            signal_end.push_back(end - 1);

            i = end;
            count++;
        }
    }

    // Calculate the new baseline by averaging the data points outside the signal regions
    int num_points = 0;
    int idx = 0;
    for (int i = 0; i < readoutWindow; i++) {
        if (idx < count && i == signal_start[idx]) {
            i = signal_end[idx];
            idx++;
        } else {
            baseline += SiPMWave[i];
            num_points++;
        }
    }

    if (num_points != 0) {
        baseline /= static_cast<float>(num_points);
    }

    // Calculate the variance of the data points outside the signal regions
    idx = 0;
    for (int i = 0; i < readoutWindow; i++) {
        if (idx < count && i == signal_start[idx]) {
            i = signal_end[idx];
            idx++;
        } else {
            variance += (SiPMWave[i] - baseline) * (SiPMWave[i] - baseline);
        }
    }

    // Calculate the standard deviation
    float std_dev = 0;
    if (num_points > 1) {
        variance /= static_cast<float>(num_points - 1);
        std_dev = std::sqrt(variance);
    }

    //std::cout << "num_points:" << num_points << "baseline:" << baseline <<"std_dev:" << std_dev<< std::endl;
    return {baseline, std_dev};
}


int main(int argc, char **argv) {
    Options options(argc, argv);
    YAML::Node const config = options.GetConfig();
    int threshold = Options::NodeAs<int>(config, {"threshold"});
    int readoutwindow = Options::NodeAs<int>(config, {"readout_window"});
    int signalstart = options.NodeAs<int>(config, {"signal_start"});
    int signalinterval = options.NodeAs<int>(config, {"signal_interval"});
    int baselineinterval = options.NodeAs<int>(config, {"baseline_interval"});

    //input file name
    std::string filename;
    if  (options.Exists("input")) {
      if (options.GetConfig()["path_prefix"])
        filename = options.NodeAs<std::string>(config, {"path_prefix"}) + "/" 
                   + options.GetAs<std::string>("input");
      else filename = options.GetAs<std::string>("input");
    }
    //output file name
    std::string outputName = "output.root";
    if  (options.Exists("output")) outputName = options.GetAs<std::string>("output");
    //reading the data file
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Unable to open file: " << filename << std::endl;
        return 1;
    }

    TFile *file1 = new TFile(outputName.c_str(), "recreate");
    TTree* tree = new TTree("evt", "evt");
    Float_t sigQ;
    Float_t bkgQ;
    Float_t baselineDev;
    tree->Branch("sigQ", &sigQ, "sigQ/F");
    tree->Branch("bkgQ", &bkgQ, "bkgQ/F");
    tree->Branch("dev", &baselineDev, "dev/F");


    std::string line;

    std::vector<float> SiPMWave;

    while (std::getline(file, line)) {
        std::istringstream iss(line);
        float num;
        while (iss >> num) 
            SiPMWave.push_back(num);
        float baseline_tmp = std::accumulate(SiPMWave.begin() , SiPMWave.begin() 
                             + baselineinterval + 1, 0) / baselineinterval;
        std::pair<float, float> BaselineAndStdDev = calculateBaselineAndStdDev(
                SiPMWave, readoutwindow, baseline_tmp + threshold);
        sigQ = std::accumulate(SiPMWave.begin() + signalstart, SiPMWave.begin() 
               + signalstart + signalinterval + 1 , 0) - BaselineAndStdDev.first * 400;
        bkgQ = std::accumulate(SiPMWave.begin(), SiPMWave.begin() + baselineinterval + 1 , 0)
               - BaselineAndStdDev.first * baselineinterval;
        baselineDev = BaselineAndStdDev.second;
        tree->Fill(); 
        SiPMWave.clear();
    }

    file1->Write();
    file1->Close();
    file.close();

    return 0;
}

