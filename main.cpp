#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <deque>
#include <limits.h>

#define PI 3.141592653589793238462643383279502884197169

using ::std::deque;

double rand_double() {
    return static_cast<double>(rand()) / RAND_MAX;
}

double rand_normal() {
    // Box-Muller method for generating normally distributed numbers
    double u = rand_double();
    double v = rand_double();
    return sqrt(-2 * log(u)) * cos(2 * PI * v);
}

double rand_normal(double mu, double sigma) {
    return rand_normal() * sigma + mu;
}

double rand_exp(double lambda) {
    double p = rand_double();
    return -log(1 - p) / lambda;
}

double average(double values[], int n) {
    double average = 0;
    for (int i = 0; i < n; ++i) {
        average += values[i] / n;
    }
    return average;
}

double standard_deviation(double values[], int n, double avg) {
    double stddev = 0;
    for (int i = 0; i < n; ++i) {
        double diff = values[i] - avg;
        stddev += diff * diff / n;
    }
    return sqrt(stddev);
}

double standard_deviation(double values[], int n) {
    double avg = average(values, n);
    return standard_deviation(values, n, avg);
}

class customer {
  private:
    static const double mean_time_per_item;
    static const double stdev_time_per_item;
    static const double mean_item_count;
    static const double stdev_item_count;

    double my_time_left;

  public:
    const int items_count;

    customer(int items, double t) : my_time_left(t), items_count(items) { }

    void decrease_time_left(double t) {
        my_time_left -= t;
    }

    double time_left() const {
        return my_time_left;
    }

    static customer random_customer() {
        int    items;
        double time;
        // keep generating random numbers of items until > 0
        do {
            items = static_cast<int>(rand_normal(mean_item_count,
                                                 stdev_item_count));
        } while (items <= 0);

        // keep generating random t ime until > 0
        do {
            // Distribution of a sum of normally distributed values is normal
            // with mean n * mu and standard deviation sigma * sqrt(n)
            time = rand_normal(mean_time_per_item * items,
                               stdev_time_per_item * sqrt(items));
        } while (time <= 0);

        return customer(items, time);
    }
};

const double customer::mean_time_per_item  = 0.1;
const double customer::stdev_time_per_item = 0.01;
const double customer::mean_item_count     = 25;
const double customer::stdev_item_count    = 10;

class lane {
  private:
    static double total_wait_time;
    static int    customers_processed;

    deque<customer> customers;
    double          time_left;

  public:
    static       int  express_limit;
           const bool is_express;

    explicit lane(bool express) : time_left(0), is_express(express) { }

    int size() const {
        return customers.size();
    }

    bool empty() const {
        return customers.empty();
    }

    void add(const customer& customer) {
        customers.push_back(customer);
        time_left += customer.time_left();
    }

    const customer& front() const {
        return customers.front();
    }

    bool can_enter(const customer& c) const {
        return is_express == false || c.items_count <= express_limit;
    }

    double total_time_left() const {
        return time_left;

    }

    void process(const double time) {
        if (customers.empty()) return;
        double front_time_left = customers.front().time_left();
        if (front_time_left <= time) {
            // First customer isn't using all the time
            // All customers had to wait for the first customer to be processed
            total_wait_time += customers.size() * front_time_left;
            // First customer is removed from the line
            customers.pop_front();
            // First customer was processed
            customers_processed++;
            // Decrease the amount of time the lane has left
            time_left -= front_time_left;
            // Recursively process the rest of the time
            process(time - front_time_left);
        } else {
            // First customer is using all the time
            // All customers had to wait for 'time'
            total_wait_time += customers.size() * time;
            // Decrease the front customers time left
            customers.front().decrease_time_left(time);
            // Decrease the amount of time the lane has left
            time_left -= time;
        }
    }

    static void reset() {
        total_wait_time     = 0;
        customers_processed = 0;
    }

    static void set_express_limit(int limit) {
        express_limit = limit;
    }

    static double average_wait_time() {
        return total_wait_time / customers_processed;
    }
};

// Initialization of lane's static variables
int    lane::customers_processed = 0;
double lane::total_wait_time     = 0;
int    lane::express_limit       = 0;

lane* get_best_line(lane* lanes[], int lanes_count, const customer& c) {
    lane* best_line  = NULL;
    double best_size = INT_MAX;
    for (int i = 0; i < lanes_count; ++i) {
        if (lanes[i]->total_time_left() < best_size && lanes[i]->can_enter(c)) {
            best_line = lanes[i];
            best_size = lanes[i]->size();
        }
    }
    return best_line;
}

static double get_average_wait_time(const int    lanes_count,
                                    const int    express_lanes_count,
                                    const double max_time,
                                    const double mean_customers_per_minute) {
    // Reset customer count and total waiting time to zero
    lane::reset();
    // Create lanes
    lane** lanes = new lane*[lanes_count];
    for (int j = 0; j < lanes_count; ++j) {
        lanes[j] = new lane(j < express_lanes_count);
    }

    for (double time = 0; time < max_time;) {
        // New customer appears after a wait of "new_time"
        double   new_time     = rand_exp(mean_customers_per_minute);
        customer new_customer = customer::random_customer();

        // All the lanes have been processing customers for "new_time"
        for (int j = 0; j < lanes_count; ++j) {
            lanes[j]->process(new_time);
        }
        // Get the best lane for the customer to enter
        lane* best_lane = get_best_line(lanes, lanes_count, new_customer);
        // Put customer in line
        best_lane->add(new_customer);
        // Increase the time
        time += new_time;
    }

    // Free resources
    for (int j = 0; j < lanes_count; ++j) {
        delete lanes[j];
    }
    delete[] lanes;
    return lane::average_wait_time();
}

int main() {
    const double mean_customers_per_minute = 4.15;
    const int    lanes_count               = 10;
    const double max_time                  = 8 * 60;  // 8 hours
    const int    iteration_count           = 10000;
          double times[iteration_count];

    // Seed the random number generator;
    srand(time(NULL));
    for (int expr_lanes_count = 0; expr_lanes_count < lanes_count; ++expr_lanes_count) {
        for (int express_limit = 0; express_limit < 50; ++express_limit) {
            lane::set_express_limit(express_limit);
            for (int i = 0; i < iteration_count; ++i) {
                times[i] = get_average_wait_time(lanes_count, expr_lanes_count,
                                                 max_time, mean_customers_per_minute);
                if(i % 100 == 0)
                    fprintf(stderr, "%f\r", 100.0 * i / iteration_count);
            }
            double avg     = average(times, iteration_count);
            double std_dev = standard_deviation(times, iteration_count, avg);
            // Print statistics
            printf("%-20d%-20d%-20f%-20f%-20d\n",
                   expr_lanes_count, express_limit, avg, std_dev, iteration_count);
            fflush(stdout);
            // If there aren't any express lanes, changing the express lane
            // limit won't matter, so stop after one iteration.
            if(expr_lanes_count == 0) break;
        }
    }
}
