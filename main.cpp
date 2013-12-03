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
//       25 items         1 customer             20 items
//  ------------------- x ---------- x 4 lanes = --------
//  1 customer * 1 lane   5 minutes              1 minute
class customer {
private:
    const static double mean_time_per_item  = 0.05;
    const static double stdev_time_per_item = 0.01;
    const static double mean_item_count     = 25;
    const static double stdev_item_count    = 5;
public:
    const int    items_count;
    const double time;
          double time_left;

    customer(int items, double t) : items_count(items), time(t), time_left(t) {
    }

    static customer random_customer() {
        int items;
        double time = 0;
        do {
            items = (int) rand_normal(mean_item_count, stdev_item_count);
        } while(items <= 0);

	for(int i = 0; i < items; ++i) {
            time += rand_normal(mean_time_per_item, stdev_time_per_item);
	}
	return customer(items, time);
    }
};

class lane {
  private:
    queue<customer> customers;
  public:
    const static int    express_limit = 15;
          static double total_wait_time;
          static int    customers_processed;

    const bool is_express;

    lane(bool express) : is_express(express) {
    }

    void process(const double time) {
        if (customers.empty()) return;
        double front_time_left = customers.front().time_left;
        if (front_time_left < time) {
            // First customer isn't using all the time
            // All customers had to wait for the first customer to be processed
            total_wait_time += customers.size() * front_time_left;
            // First customer is removed from the line
            customers.pop();
            // Front customer was processed
            customers_processed++;
            // process the rest of the time
            process(time - front_time_left);
	} else {
            // First customer is using all the time
            // All customers had to wait for 'time'
            total_wait_time             += customers.size() * time;
            // Decrease the front customers time left
            customers.front().time_left -= time;
        }
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

    static void reset() {
        total_wait_time = 0;
        customers_processed = 0;

    }
};

int    lane::customers_processed = 0;
double lane::total_wait_time     = 0;

lane* get_best_line(lane* lanes[], int lanes_count, int items) {
    lane* best_line = NULL;
    int best_size = 10E6;
    for (int i = 1; i < lanes_count; ++i) {
        if (lanes[i]->size() < best_size &&
           (lanes[i]->is_express == false ||
            lanes[i]->is_express && items < lane::express_limit)) {
            best_line = lanes[i];
            best_size = lanes[i]->size();
        }
    }
    return best_line;
}

int main() {
    const double mean_time_between_customers = 1;
    const int    lanes_count                 = 5;
    const int    express_lanes_count         = 1;
    const double max_time                    = 8 * 60;

    // Seed the random number generator;
    srand(time(NULL));

    double k = 0;

    for(int i = 0; i < 10000; ++i) {
        lane::reset();
        lane* lines[lanes_count];
        for(int j = 0; j < lanes_count; ++j) {
            lines[j] = new lane(j < express_lanes_count);
        }

        double time = 0;
        while (time < max_time) {
            customer new_customer = customer::random_customer();
            double   new_time     = rand_exp(1 / mean_time_between_customers);
            for(int i = 0; i < lanes_count; ++i) {
                lines[i]->process(new_time);
            }
            lane* best_line = get_best_line(lines, lanes_count, new_customer.items_count);

            // put customer in line
            best_line->add(new_customer);
            time += new_time;
        }

        // printf("%-50s %d\n", "Customers processed:", line::customers_processed);
        // printf("%-50s %f\n", "Total wait time:",     line::total_wait_time);
        // printf("%-50s %f\n", "Mean wait time:",      line::total_wait_time / line::customers_processed);
        // printf("%f\n", lane::total_wait_time / lane::customers_processed);
        k += lane::total_wait_time / lane::customers_processed;
        printf("%f\n", k / (i + 1));
        for(int i = 0; i < lanes_count; ++i) {
            delete lines[i];
        }
    }
}

