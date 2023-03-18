#include <iostream>
#include <numeric>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>
#include <random>
#include <memory>

// Include ROOT headers
#include "TTree.h"
#include "TFile.h"

// Include User-defined headers
#include "ExceptionV.h"
#include "Logger.h"
#include "Options.h"

//This function reads the SiPM single waveform (2002 points)
// and automatically calculate the baseline position as well
// as the standard deviation corresponding to the baseline choice
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

//The function reads information from a line of csv file
// or just a std::string with csvLine[i] represented for
// different information of the test
void readAndFill(const YAML::Node config, std::string csvLine, std::unique_ptr<TTree> tree) {
    int threshold = Options::NodeAs<int>(config, {"threshold"});
    int readoutwindow = Options::NodeAs<int>(config, {"readout_window"});
    int signalstart = Options::NodeAs<int>(config, {"signal_start"});
    int signalinterval = Options::NodeAs<int>(config, {"signal_interval"});
    int baselineinterval = Options::NodeAs<int>(config, {"baseline_interval"});
    std::vector<std::string> elements;
    std::stringstream ss(csvLine);
    std::string element;
    while (std::getline(ss, element, ',')) {
        elements.push_back(element);
    }
    LOG_INFO << "Starting to process SiPM No." << elements[0] << " at the voltage " << elements[1];
    tree.reset(new TTree(("SiPM_" + elements[0] + "_" + elements[1]).c_str(), "events"));
    Float_t sigQ;
    Float_t bkgQ;
    Float_t baselineDev;
    tree->Branch("sigQ", &sigQ, "sigQ/F");
    tree->Branch("bkgQ", &bkgQ, "bkgQ/F");
    tree->Branch("dev", &baselineDev, "dev/F");
    std::ifstream file(elements[2]);
    if (!file.is_open()) {
        LOG_ERROR << "Unable to open file: " << elements[2] << std::endl;
        return;
    }
    std::string line;

    std::vector<float> SiPMWave;

    unsigned long countLine = 0;
    while (std::getline(file, line)) {
        if (countLine % 10000 == 0) LOG_INFO << countLine << " lines have been processed.";
        countLine += 1;
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
    tree->Write();
}


int main(int argc, char **argv) {
    Options options(argc, argv);
    YAML::Node const config = options.GetConfig();

    //output file name
    std::string outputName = "output.root";
    if  (options.Exists("output")) outputName = options.GetAs<std::string>("output");
    TFile *file1 = new TFile(outputName.c_str(), "recreate");
    std::unique_ptr<TTree> tree_ptr;

    //input file name
    std::string filename;
    if  (options.Exists("input")) {
        if (options.GetConfig()["path_prefix"])
            filename = options.NodeAs<std::string>(config, {"path_prefix"}) + "/" 
                       + options.GetAs<std::string>("input");
        else filename = options.GetAs<std::string>("input");

        std::ifstream infile(filename);
        if (!infile.is_open()) {
            LOG_ERROR << "Program abort. Could not open csv file.";
            std::abort();
        }
        std::string line;
        while (std::getline(infile, line)) {
            readAndFill(config, line, std::move(tree_ptr)); 
        }
    }
    else if (options.Exists("dataset")) {
        std::string testLine;
        testLine = options.GetAs<std::string>("dataset");
        readAndFill(config, testLine, std::move(tree_ptr));
    }
    else
        LOG_ERROR << "cannot use both input datasets csv file and a single line!";

    file1->Close();

    return 0;
}
