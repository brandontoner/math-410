#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <queue>

#define PI 3.141592653589793238462643383279502884197169

using ::std::queue;

double rand_double() {
    return (double) rand() / RAND_MAX;
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

//  25 items      0.1 minutes     4.15 customers    10 lanes
// ---------- x ---------------- x ------------- x
// 1 customer   1 item  * 1 lane     1 minute
class customer {
private:
    const static double mean_time_per_item  = 0.1;
    const static double stdev_time_per_item = 0.01;
    const static double mean_item_count     = 25;
    const static double stdev_item_count    = 10;

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
            items = (int) rand_normal(mean_item_count, stdev_item_count);
        } while(items <= 0);

        // Distribution of a sum of normally distributed values is normal with
        // mean n * mu and standard deviation sigma * sqrt(n)
        double time = rand_normal(mean_time_per_item * items,
                                  stdev_time_per_item * sqrt(items));

	// for(int i = 0; i < items; ++i) {
        //     time += rand_normal(mean_time_per_item, stdev_time_per_item);
	// }
	return customer(items, time);
    }
};

class lane {
  private:
    static double total_wait_time;
    static int    customers_processed;

    queue<customer> customers;
  public:
    const static int  express_limit = 25;
    const        bool is_express;

    lane(bool express) : is_express(express) {
    }

    int size() const {
        return customers.size();
    }

    bool empty() const {
        return customers.empty();
    }

    void add(const customer& customer) {
        customers.push(customer);
    }

    const customer& front() const {
        return customers.front();
    }

    bool can_enter(const customer& c) const {
        return is_express == false || c.items_count <= express_limit;
    }

    void process(const double time) {
        if (customers.empty()) return;
        double front_time_left = customers.front().time_left();
        if (front_time_left <= time) {
            // First customer isn't using all the time
            // All customers had to wait for the first customer to be processed
            total_wait_time += customers.size() * front_time_left;
            // First customer is removed from the line
            customers.pop();
            // Front customer was processed
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
        total_wait_time = 0;
        customers_processed = 0;
    }

    static double average_wait_time() {
        if (customers_processed == 0) throw 10;
        return total_wait_time / customers_processed;
    }
};

int    lane::customers_processed = 0;
double lane::total_wait_time     = 0;

lane* get_best_line(lane* lanes[], int lanes_count, const customer& c) {
    lane* best_line = NULL;
    int best_size = 2E9;
    for (int i = 0; i < lanes_count; ++i) {
        if (lanes[i]->size() < best_size && lanes[i]->can_enter(c)) {
            best_line = lanes[i];
            best_size = lanes[i]->size();
        }
    }
    return best_line;
}

int main() {
    const double mean_customers_per_minute = 4.15;
    const int    lanes_count               = 10;
    const int    express_lanes_count       = 2;
    const double max_time                  = 8 * 60; // 8 hours
    const int    iteration_count           = 100000;
          double times[iteration_count];

    // Seed the random number generator;
    srand(time(NULL));


    for(int i = 0; i < iteration_count; ++i) {
        // Reset customer count and total waiting time to zero
        lane::reset();
        // Create lanes
        lane* lanes[lanes_count];
        for(int j = 0; j < lanes_count; ++j) {
            lanes[j] = new lane(j < express_lanes_count);
        }

        for(double time = 0; time < max_time;) {
            // New customer appears after a wait of "new_time"
            double   new_time     = rand_exp(mean_customers_per_minute);
            customer new_customer = customer::random_customer();

            // All the lanes have been processing customers for "new_time"
            for(int j = 0; j < lanes_count; ++j) {
                lanes[j]->process(new_time);
            }
            // Get the best lane for the customer to enter
            lane* best_lane = get_best_line(lanes, lanes_count, new_customer);
            // Put customer in line
            best_lane->add(new_customer);
            // Increase the time
            time += new_time;
        }

        times[i] = lane::average_wait_time();
        printf("%f\n", times[i]);

        // Free resources
        for(int j = 0; j < lanes_count; ++j) {
            delete lanes[j];
        }
    }

    double avg = average(times, iteration_count);
    double stddev  = standard_deviation(times, iteration_count, avg);
    printf("avg %0.4f std-dev %0.4f\n", avg, stddev);
}

