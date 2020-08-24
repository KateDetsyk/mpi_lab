#include <iostream>
#include <string>
#include <cmath>
#include <boost/mpi.hpp>
#include <boost/multi_array.hpp>
#include <png++/png.hpp>
#include "other.h"


void create_init_array(boost::multi_array<double, 2>& arr, Configuration& conf) {
    double value = 0.0;
    for (size_t i = 0; i != conf.size_x; ++i) {
        for (size_t j = 0; j != conf.size_y; ++j) {
            if ((i == 0) && (j < conf.size_y-1)) { arr[i][j] = conf.left_border; }
            else if ((i < conf.size_x-1) && (j == conf.size_y-1)) { arr[i][j] = conf.bottom_border; }
            else if ((i == conf.size_x-1) && (j > 0)) { arr[i][j] = conf.right_border; }
            else if ((i > 0) && (j == 0)) { arr[i][j] = conf.top_border; }
            else { arr[i][j] = value; }
        }
    }
}

void create_image(boost::multi_array<double, 2>& array, double& border_temperature, std::string& name) {
    png::image< png::rgb_pixel > image(array.shape()[0], array.shape()[1]);
    int color = 255;
    for (size_t y = 0; y < image.get_height(); ++y) {
        for (size_t x = 0; x < image.get_width(); ++x) {
            //violet = the hottest, green = colder;
            image[y][x] = png::rgb_pixel(color * (array[x][y] / border_temperature ),     //red
                                         color * (1 - array[x][y] / border_temperature),  //green
                                         color * (array[x][y] / border_temperature ));    //blue
        }
    }
    image.write(name);
}

std::vector<std::pair<int, int>> slice_array(int child_processes_number, size_t len) {
    std::vector<std::pair<int, int>> slices(child_processes_number);
    size_t start = 0;
    size_t part = len/child_processes_number;

    for (int i = 1; i <= child_processes_number; i++) {
        if (i == child_processes_number) { slices[i - 1] = {start, len-1}; }
        else {
            slices[i - 1] = {start, (i * part) + 1};
            start = i * part;
        }
    }
    return slices;
}

void new_temperature(boost::multi_array<double, 2>& old_states, boost::multi_array<double, 2>& new_states,
        Configuration& conf) {
    double alpha = conf.conductivity / (conf.density * conf.capacity); // alpha = k/C*p
    for (size_t i = 1; i < old_states.shape()[0] - 1; i++) {
        for (size_t j = 1; j < old_states.shape()[1] -1; j++) {
            new_states[i][j] = old_states[i][j] + (conf.delta_t * alpha) * (
                    ((old_states[i-1][j] - 2 * old_states[i][j] + old_states[i+1][j])/(conf.delta_x * conf.delta_x)) +
                    ((old_states[i][j-1] - 2 * old_states[i][j] + old_states[i][j+1])/(conf.delta_y * conf.delta_y)));
        }
    }
}

void rows_exchange(boost::mpi::communicator& world, int process, boost::multi_array<double, 2>& array,
                   std::pair<size_t, size_t>& indexes) {
    boost::mpi::request reqs[2];
    boost::multi_array<double, 1> send_row = array[boost::indices[indexes.first][boost::multi_array_types::index_range()]];
    reqs[0] = world.isend(process, 0, &send_row[0], send_row.shape()[0]);
    boost::multi_array<double, 1> recv_row(boost::extents[array.shape()[1]]);
    reqs[1] = world.irecv(process, 0, &recv_row[0], array.shape()[1]);
    boost::mpi::wait_all(reqs + 1, reqs + 2);
    array[indexes.second] = recv_row;
}

void recv_results(boost::mpi::communicator& world, int child_processes_number,
        boost::multi_array<double, 2>& result_arr, std::vector<std::pair<int, int>>& borders) {
    boost::mpi::request reqs [child_processes_number];

    for (int i = 0; i < child_processes_number; i++) {
        reqs[i] = world.irecv((i+1), 0, &result_arr[borders[i].first][0], (
                (borders[i].second - borders[i].first + 1 ) * result_arr.shape()[1]));
    }
    boost::mpi::wait_all(reqs, reqs + child_processes_number);
}


int main(int argc, char * argv[]) {
    boost::mpi::environment env{argc, argv};
    boost::mpi::communicator world;

    if (world.size() < 2) { throw std::runtime_error("Incorrect amount of processes! Try 2 or more."); }
    const int child_processes_number = world.size() - 1;

    // conf read
    std::string config_file;
    if (argc == 2){config_file = argv[1];}
    else if (argc == 1){config_file = "../conf.dat";}
    else{throw std::runtime_error("Incorrect amount of arguments!");}

    Configuration configuration;
    std::ifstream config_stream(config_file);
    if(!config_stream.is_open()){
        throw std::runtime_error("Failed to open configurations file " + config_file);
    }
    configuration = read_conf(config_stream);

    // main process
    if (world.rank() == 0) {
        // check Von Neumann criterion
        double alpha = configuration.conductivity / (configuration.density * configuration.capacity);
        if (configuration.delta_t > (pow(std::max(configuration.delta_x, configuration.delta_y), 2)/(4 * alpha))) {
            throw std::runtime_error("Von Neumann criterion wasn't satisfied!");
        }

        //create initial array
        boost::multi_array<double, 2> init_array(boost::extents[configuration.size_x][configuration.size_y]);
        create_init_array(init_array, configuration);

        //send parts of array to child processes
        std::vector<std::pair<int, int>> borders = slice_array(child_processes_number, configuration.size_x);
        boost::mpi::request reqs[child_processes_number];

        for (int i = 0; i < child_processes_number; i++) {
            auto arr_to_send = init_array[boost::indices
            [boost::multi_array_types::index_range().start(borders[i].first).finish(borders[i].second + 1)]
            [boost::multi_array_types::index_range()]];
            reqs[i] = world.isend((i + 1), 0, &arr_to_send[0][0], arr_to_send.num_elements());
        }
        boost::mpi::wait_all(reqs, reqs + child_processes_number);

        //get results
        boost::multi_array<double, 2> result(boost::extents[configuration.size_x][configuration.size_y]);

        double max_temperature = std::max({configuration.left_border, configuration.top_border,
                                           configuration.right_border, configuration.bottom_border}, comp);
        // every "save" interval new image
        for (int i = 0; i < configuration.maxTime/configuration.save; i++) {
            recv_results(world, child_processes_number, result, borders);
            std::string name = configuration.img_folder + "/" + std::to_string(i) + ".png";
            create_image(result, max_temperature, name);
        }
    } else { // child processes
        int process = world.rank();
        std::vector<std::pair<int, int>> borders = slice_array(child_processes_number, configuration.size_x);
        size_t x = borders[process - 1].second - borders[process - 1].first + 1;
        boost::multi_array<double, 2> process_arr(boost::extents[x][configuration.size_y]);

        // receive its part of matrix
        world.recv(0, 0, &process_arr[0][0], process_arr.num_elements());

        // change temperature
        boost::multi_array<double, 2> calculated_arr = process_arr;
        size_t counter = 0;
        for (size_t i = 1; (configuration.delta_t * i) <= configuration.maxTime; i++) {
            new_temperature(process_arr, calculated_arr, configuration);
            std::swap(process_arr, calculated_arr);

            if (process < child_processes_number) {
                //send second from end & recv last row to/from bigger process
                std::pair<size_t, size_t> indexes = {process_arr.shape()[0] - 2, process_arr.shape()[0] - 1}; // {toSend, toRecv}
                rows_exchange(world, process+1, process_arr, indexes);
            }
            if (process > 1) {
                //send second row & recv first row to/from smaller process
                std::pair<size_t, size_t> indexes = {1, 0};  // {toSend, toRecv}
                rows_exchange(world, process-1, process_arr, indexes);
            }
            // save result every n iterations, conf.save
            counter += configuration.delta_t;
            if((counter + configuration.delta_t) > configuration.save) {
                counter = 0;
                world.send(0, 0, &process_arr[0][0], process_arr.num_elements());
            }
        }
    }
    return 0;
}
