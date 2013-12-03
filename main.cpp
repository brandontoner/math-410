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
    static const double mean_time_per_item  = 0.1;
    static const double stdev_time_per_item = 0.01;
    static const double mean_item_count     = 25;
    static const double stdev_item_count    = 10;

    double my_time_left;

  public:
    const int items_count;

    customer(int items, double t) : items_count(items), my_time_left(t) { }

    void decrease_time_left(double t) {
        my_time_left -= t;
    }

    double time_left() const {
        return my_time_left;
    }

    static customer random_customer() {
        int items;
        do {
            items = static_cast<int>(rand_normal(mean_item_count,
                                                 stdev_item_count));
        } while (items <= 0);

        // Distribution of a sum of normally distributed values is normal with
        // mean n * mu and standard deviation sigma * sqrt(n)
        double time = rand_normal(mean_time_per_item * items,
                                  stdev_time_per_item * sqrt(items));

        return customer(items, time);
    }
};

class lane {
  private:
    static double total_wait_time;
    static int    customers_processed;

    deque<customer> customers;

  public:
    static       int  express_limit;
           const bool is_express;

    explicit lane(bool express) : is_express(express) { }

    int size() const {
        return customers.size();
    }

    bool empty() const {
        return customers.empty();
    }

    void add(const customer& customer) {
        customers.push_back(customer);
    }

    const customer& front() const {
        return customers.front();
    }

    bool can_enter(const customer& c) const {
        return is_express == false || c.items_count <= express_limit;
    }

    double total_time_left() const {
        double time = 0;
        std::deque<customer>::const_iterator it = customers.begin();
        for(; it != customers.end(); ++it) {
            time += it->time_left();
        }
        return time;
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
            // Recursively process the rest of the time
            process(time - front_time_left);
        } else {
            // First customer is using all the time
            // All customers had to wait for 'time'
            total_wait_time += customers.size() * time;
            // Decrease the front customers time left
            customers.front().decrease_time_left(time);
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

static double get_average_wait_time(const int lanes_count,
                                    const int express_lanes_count,
                                    const double max_time,
                                    const double mean_customers_per_minute) {
    // Reset customer count and total waiting time to zero
    lane::reset();
        // Create lanes
        lane* lanes[lanes_count];
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
        return lane::average_wait_time();
}

int main() {
    const double mean_customers_per_minute = 4.15;
    const int    lanes_count               = 10;
    const int    express_lanes_count       = 1;
    const double max_time                  = 8 * 60;  // 8 hours
    const int    iteration_count           = 5000;
    // const int    express_limit             = 25;
          double times[iteration_count];

    // Seed the random number generator;
    srand(time(NULL));

    for(int exp = 0; exp < 50; ++exp) {
        lane::set_express_limit(exp);
        for (int i = 0; i < iteration_count; ++i) {
            times[i] = get_average_wait_time(lanes_count, express_lanes_count,
                                             max_time, mean_customers_per_minute);
            // if(i % 100 == 0)
            //    fprintf(stderr, "%f\r", 100.0 * i / iteration_count);
        }
        double avg     = average(times, iteration_count);
        double stddev  = standard_deviation(times, iteration_count, avg);
        printf("%d\t%d\t%f\t%f\n", express_lanes_count, exp, avg, stddev);
    }

//    double avg     = average(times, iteration_count);
//    double stddev  = standard_deviation(times, iteration_count, avg);
//    printf("average:            %0.4f\n"
//           "standard deviation: %0.4f\n", avg, stddev);
//    printf("confidence interval:\n"
//           "%0.4f %0.4f\n", avg - 2 * stddev / sqrt(iteration_count), avg + 2 * stddev / sqrt(iteration_count));
}
