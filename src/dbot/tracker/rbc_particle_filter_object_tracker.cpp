/*
 * This is part of the Bayesian Object Tracking (bot),
 * (https://github.com/bayesian-object-tracking)
 *
 * Copyright (c) 2015 Max Planck Society,
 * 				 Autonomous Motion Department,
 * 			     Institute for Intelligent Systems
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License License (GNU GPL). A copy of the license can be found in the LICENSE
 * file distributed with this source code.
 */

#include <dbot/tracker/rbc_particle_filter_object_tracker.hpp>

namespace dbot
{
RbcParticleFilterObjectTracker::RbcParticleFilterObjectTracker(
    const std::shared_ptr<Filter>& filter,
    const dbot::ObjectModel& object_model,
    const dbot::CameraData& camera_data,
    int evaluation_count,
    double update_rate)
    : ObjectTracker(object_model, camera_data, update_rate),
      filter_(filter),
      evaluation_count_(evaluation_count)
{
}

auto RbcParticleFilterObjectTracker::on_initialize(
    const std::vector<State>& initial_states) -> State
{
    filter_->set_particles(initial_states);
    filter_->filter(camera_data_.depth_image(), zero_input());
    filter_->resample(evaluation_count_ / object_model_.count_parts());

    State delta_mean = filter_->belief().mean();

    for (size_t i = 0; i < filter_->belief().size(); i++)
    {
        filter_->belief().location(i).center_around_zero(delta_mean);
    }

    auto& integrated_poses = filter_->observation_model()->integrated_poses();
    integrated_poses.apply_delta(delta_mean);

    return integrated_poses;
}

auto RbcParticleFilterObjectTracker::on_track(const Obsrv& image) -> State
{
    filter_->filter(image, zero_input());

    State delta_mean = filter_->belief().mean();

    for (size_t i = 0; i < filter_->belief().size(); i++)
    {
        filter_->belief().location(i).center_around_zero(delta_mean);
    }

    auto& integrated_poses = filter_->observation_model()->integrated_poses();
    integrated_poses.apply_delta(delta_mean);

    return integrated_poses;
}
}