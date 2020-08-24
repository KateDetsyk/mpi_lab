#ifndef MPI_OTHER_H
#define MPI_OTHER_H

#endif //MPI_OTHER_H
#include <fstream>
#include <string>


struct Configuration
{
    double capacity;
    double conductivity;
    double density;
    size_t size_x;
    size_t size_y;
    double delta_x;
    double delta_y;
    size_t delta_t;
    size_t save;
    size_t maxTime;
    double left_border;
    double top_border;
    double right_border;
    double bottom_border;
    std::string img_folder;
};

Configuration read_conf(std::ifstream &config_stream){
    Configuration conf;
    std::map<std::string, std::string> confKeywords;
    std::string tempKey;
    std::string t;
    while(config_stream >> tempKey){
        config_stream >> t;
        confKeywords.insert(std::pair<std::string, std::string>(tempKey, t));
    }
    config_stream.close();
    std::stringstream(confKeywords["capacity"])>>conf.capacity;
    std::stringstream(confKeywords["conductivity"])>>conf.conductivity;
    std::stringstream(confKeywords["density"])>>conf.density;
    std::stringstream(confKeywords["size_x"])>>conf.size_x;
    std::stringstream(confKeywords["size_y"])>>conf.size_y;
    std::stringstream(confKeywords["delta_x"])>>conf.delta_x;
    std::stringstream(confKeywords["delta_y"])>>conf.delta_y;
    std::stringstream(confKeywords["delta_t"])>>conf.delta_t;
    std::stringstream(confKeywords["save"])>>conf.save;
    std::stringstream(confKeywords["left_border"])>>conf.left_border;
    std::stringstream(confKeywords["top_border"])>>conf.top_border;
    std::stringstream(confKeywords["right_border"])>>conf.right_border;
    std::stringstream(confKeywords["bottom_border"])>>conf.bottom_border;
    std::stringstream(confKeywords["maxTime"])>>conf.maxTime;
    conf.img_folder = confKeywords["img_folder"];

    return conf;
}

bool comp(double a, double b) {return (a < b);}

