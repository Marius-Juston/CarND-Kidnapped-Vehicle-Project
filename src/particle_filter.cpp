/**
 * particle_filter.cpp
 *
 * Created on: Dec 12, 2016
 * Author: Tiffany Huang
 */

#include "particle_filter.h"

#include <cmath>
#include <algorithm>
#include <iostream>
#include <iterator>
#include <numeric>
#include <random>
#include <string>
#include <vector>

#include "helper_functions.h"

using std::string;
using std::vector;
using std::normal_distribution;

void ParticleFilter::init(double x, double y, double theta, double std[]) {
  /**
   * TODO: Set the number of particles. Initialize all particles to 
   *   first position (based on estimates of x, y, theta and their uncertainties
   *   from GPS) and all weights to 1. 
   * TODO: Add random Gaussian noise to each particle.
   * NOTE: Consult particle_filter.h for more information about this method 
   *   (and others in this file).
   */
  num_particles = 5;  // TODO: Set the number of particles

  // This line creates a normal (Gaussian) distribution for x
  normal_distribution<double> dist_x(x, std[0]);
  normal_distribution<double> dist_y(y, std[1]);
  normal_distribution<double> dist_theta(theta, std[2]);

  double sample_x, sample_y, sample_theta;

  for (int i = 0; i < num_particles; ++i) {
    sample_x = dist_x(gen);
    sample_y = dist_y(gen);
    sample_theta = dist_theta(gen);

    Particle particle;
    particle.x = sample_x;
    particle.y = sample_y;
    particle.theta = sample_theta;
    particle.id = i;
    particle.weight = 1.0;

    std::cout << particle.id << '\t' << particle.x << '\t' << particle.y << '\t' << particle.theta << std::endl;

    particles.push_back(particle);
  }

  is_initialized = true;
}

void ParticleFilter::prediction(double delta_t, double std_pos[],
                                double velocity, double yaw_rate) {
  /**
   * TODO: Add measurements to each particle and add random Gaussian noise.
   * NOTE: When adding noise you may find std::normal_distribution 
   *   and std::default_random_engine useful.
   *  http://en.cppreference.com/w/cpp/numeric/random/normal_distribution
   *  http://www.cplusplus.com/reference/random/default_random_engine/
   */

  normal_distribution<double> dist_x(0, std_pos[0]);
  normal_distribution<double> dist_y(0, std_pos[1]);
  normal_distribution<double> dist_theta(0, std_pos[2]);

  double x;
  double y;
  double theta;

  for (auto &particle : particles) {
    if (yaw_rate == 0) {
      x = particle.x + velocity * delta_t * cos(particle.theta);
      y = particle.y + velocity * delta_t * sin(particle.theta);
      theta = particle.theta;
    } else {
      x = particle.x + velocity / yaw_rate * (sin(particle.theta + yaw_rate * delta_t) - sin(particle.theta));
      y = particle.y + velocity / yaw_rate * (cos(particle.theta) - cos(particle.theta + yaw_rate * delta_t));
      theta = particle.theta + yaw_rate * delta_t;
    }

    x += dist_x(gen);
    y += dist_y(gen);
    theta += dist_theta(gen);

    particle.x = x;
    particle.y = y;
    particle.theta = theta;

//    std::cout << particle.id << '\t' << particle.x << '\t' << particle.y << '\t' << particle.theta << '\t'
//              << particle.weight << std::endl;
  }
}

void ParticleFilter::dataAssociation(Particle &particle,
                                     const Map &map_landmarks) {
  /**
   * TODO: Find the predicted measurement that is closest to each 
   *   observed measurement and assign the observed measurement to this 
   *   particular landmark.
   * NOTE: this method will NOT be called by the grading code. But you will 
   *   probably find it useful to implement this method and use it as a helper 
   *   during the updateWeights phase.
   */

  float x;
  float y;
  double distance;
  particle.associations.clear();

  for (int i = 0; i < particle.sense_x.size(); ++i) {
    x = particle.sense_x[i];
    y = particle.sense_y[i];
    int index = 0;
    double min_dist = dist(x, y, map_landmarks.landmark_list[0].x_f, map_landmarks.landmark_list[0].y_f);

    for (int j = 1; j < map_landmarks.landmark_list.size(); ++j) {
      distance = dist(x, y, map_landmarks.landmark_list[j].x_f, map_landmarks.landmark_list[j].y_f);

      if (distance < min_dist) {
        min_dist = distance;
        index = j;
      }
    }

    particle.associations.push_back(index + 1);
  }
}

void ParticleFilter::updateWeights(double sensor_range, double std_landmark[],
                                   const vector<LandmarkObs> &observations,
                                   const Map &map_landmarks) {
  /**
   * TODO: Update the weights of each particle using a mult-variate Gaussian 
   *   distribution. You can read more about this distribution here: 
   *   https://en.wikipedia.org/wiki/Multivariate_normal_distribution
   * NOTE: The observations are given in the VEHICLE'S coordinate system. 
   *   Your particles are located according to the MAP'S coordinate system. 
   *   You will need to transform between the two systems. Keep in mind that
   *   this transformation requires both rotation AND translation (but no scaling).
   *   The following is a good resource for the theory:
   *   https://www.willamette.edu/~gorr/classes/GeneralGraphics/Transforms/transforms2d.htm
   *   and the following is a good resource for the actual equation to implement
   *   (look at equation 3.33) http://planning.cs.uiuc.edu/node99.html
   */

  double x;
  double y;
  weights.clear();

  for (auto &particle : particles) {
    particle.sense_x.clear();
    particle.sense_y.clear();

    for (auto observation : observations) {
      x = particle.x + (observation.x * cos(particle.theta)) - (observation.y * sin(particle.theta));
      y = particle.y + (observation.x * sin(particle.theta)) + (observation.y * cos(particle.theta));

//      std::cout << round(particle.x) << '\t'
//                << round(particle.y) << '\t'
//                << round(observation.x) << '\t'
//                << round(observation.y) << '\t'
//                << round(particle.theta * 180. / M_PI) << '\t'
//                << round(x) << '\t'
//                << round(y) << std::endl;

      particle.sense_x.push_back(x);
      particle.sense_y.push_back(y);
    }
    dataAssociation(particle, map_landmarks);

    particle.weight = getWeight(particle, map_landmarks, std_landmark);
    weights.push_back(particle.weight);

  }
}

void ParticleFilter::resample() {
  /**
   * TODO: Resample particles with replacement with probability proportional 
   *   to their weight. 
   * NOTE: You may find std::discrete_distribution helpful here.
   *   http://en.cppreference.com/w/cpp/numeric/random/discrete_distribution
   */

  vector<Particle> newParticles(num_particles);

  std::discrete_distribution<> index_gen(weights.begin(), weights.end());

  for (int _ = 0; _ < num_particles; ++_) {
    newParticles.push_back(particles[index_gen(gen)]);
  }

  particles = newParticles;

}

void ParticleFilter::SetAssociations(Particle &particle,
                                     const vector<int> &associations,
                                     const vector<double> &sense_x,
                                     const vector<double> &sense_y) {
  // particle: the particle to which assign each listed association, 
  //   and association's (x,y) world coordinates mapping
  // associations: The landmark id that goes along with each listed association
  // sense_x: the associations x mapping already converted to world coordinates
  // sense_y: the associations y mapping already converted to world coordinates
  particle.associations = associations;
  particle.sense_x = sense_x;
  particle.sense_y = sense_y;
}

string ParticleFilter::getAssociations(const Particle &best) {
  vector<int> v = best.associations;
  std::stringstream ss;
  copy(v.begin(), v.end(), std::ostream_iterator<int>(ss, " "));
  string s = ss.str();
  s = s.substr(0, s.length() - 1);  // get rid of the trailing space
  return s;
}

string ParticleFilter::getSenseCoord(const Particle &best, const string &coord) {
  vector<double> v;

  if (coord == "X") {
    v = best.sense_x;
  } else {
    v = best.sense_y;
  }

  std::stringstream ss;
  copy(v.begin(), v.end(), std::ostream_iterator<float>(ss, " "));
  string s = ss.str();
  s = s.substr(0, s.length() - 1);  // get rid of the trailing space
  return s;
}
double ParticleFilter::getWeight(const Particle &particle, const Map &map, const double position_std[]) {
  double x;
  double y;
  double landmark_x;
  double landmark_y;
  double x_std = position_std[0];
  double y_std = position_std[1];
  int index;
  double weight = 1;

//  std::cout << particle.associations.size() << std::endl;

  for (int i = 0; i < particle.associations.size(); ++i) {
    x = particle.sense_x[i];
    y = particle.sense_y[i];

    index = particle.associations[i] - 1;

    landmark_x = map.landmark_list[index].x_f;
    landmark_y = map.landmark_list[index].y_f;

    double w = exp(-(pow(x - landmark_x, 2) / (2 * x_std * x_std) + pow(y - landmark_y, 2) / (2 * y_std * y_std)))
        / (2 * M_PI * x_std * y_std);

//    std::cout << index << "\t\t" << landmark_x << "\t\t" << landmark_y << "\t\t" << w << std::endl;

    weight *= w;
  }

  return weight;
}
