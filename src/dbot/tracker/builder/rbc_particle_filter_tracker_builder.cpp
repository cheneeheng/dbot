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

#include <dbot/util/simple_wavefront_object_loader.hpp>
#include <dbot/tracker/builder/rbc_particle_filter_tracker_builder.hpp>

#ifdef DBOT_BUILD_GPU
#include <dbot/tracker/builder/rb_observation_model_gpu_builder.hpp>
#endif

namespace dbot
{
RbcParticleFilterTrackerBuilder::RbcParticleFilterTrackerBuilder(
    const Parameters& param,
    const CameraData& camera_data)
    : param_(param), camera_data_(camera_data)
{
}

std::shared_ptr<RbcParticleFilterObjectTracker>
RbcParticleFilterTrackerBuilder::build()
{
    auto object_model = create_object_model(param_.ori);
    auto filter = create_filter(object_model, param_.tracker.max_kl_divergence);

    auto tracker = std::make_shared<RbcParticleFilterObjectTracker>(
        filter,
        object_model,
        camera_data_,
        param_.tracker.evaluation_count,
        param_.tracker.update_rate);

    return tracker;
}

auto RbcParticleFilterTrackerBuilder::create_filter(
    const ObjectModel& object_model,
    double max_kl_divergence) -> std::shared_ptr<Filter>
{
    //    auto state_transition_model =
    //        create_brownian_state_transition_model(param_.state_transition);

    auto state_transition_model =
        create_object_transition_model(param_.object_transition);

    auto obsrv_model = create_obsrv_model(
        param_.use_gpu, object_model, camera_data_, param_.observation);

    auto sampling_blocks = create_sampling_blocks(
        object_model.count_parts(),
        state_transition_model->noise_dimension() / object_model.count_parts());

    auto filter = std::shared_ptr<Filter>(new Filter(state_transition_model,
                                                     obsrv_model,
                                                     sampling_blocks,
                                                     max_kl_divergence));

    return filter;
}

// auto RbcParticleFilterTrackerBuilder::create_brownian_state_transition_model(
//    const BrownianMotionModelBuilder<State, Input>::Parameters& param) const
//    -> std::shared_ptr<StateTransition>
//{
//    BrownianMotionModelBuilder<State, Input> process_builder(param);
//    std::shared_ptr<StateTransition> process = process_builder.build();

//    return process;
//}

auto RbcParticleFilterTrackerBuilder::create_object_transition_model(
    const ObjectTransitionModelBuilder<
        RbcParticleFilterTrackerBuilder::State,
        RbcParticleFilterTrackerBuilder::Input>::Parameters& param) const
    -> std::shared_ptr<StateTransition>
{
    ObjectTransitionModelBuilder<State, Input> process_builder(param);
    std::shared_ptr<StateTransition> process = process_builder.build();

    return process;
}

auto RbcParticleFilterTrackerBuilder::create_obsrv_model(
    bool use_gpu,
    const ObjectModel& object_model,
    const CameraData& camera_data,
    const RbObservationModelBuilder<State>::Parameters& param) const
    -> std::shared_ptr<ObservationModel>
{
    std::shared_ptr<ObservationModel> obsrv_model;

    if (!use_gpu)
    {
        obsrv_model = RbObservationModelCpuBuilder<State>(
                          param, object_model, camera_data)
                          .build();
    }
    else
    {
#ifdef DBOT_BUILD_GPU
        obsrv_model = RbObservationModelGpuBuilder<State>(
                          param, object_model, camera_data)
                          .build();
#else
        throw NoGpuSupportException();
#endif
    }

    return obsrv_model;
}

ObjectModel RbcParticleFilterTrackerBuilder::create_object_model(
    const ObjectResourceIdentifier& ori) const
{
    ObjectModel object_model;

    object_model.load_from(std::shared_ptr<ObjectModelLoader>(
                               new SimpleWavefrontObjectModelLoader(ori)),
                           true);

    return object_model;
}

std::vector<std::vector<size_t>>
RbcParticleFilterTrackerBuilder::create_sampling_blocks(int blocks,
                                                        int block_size) const
{
    std::vector<std::vector<size_t>> sampling_blocks(param_.ori.count_meshes());
    for (int i = 0; i < blocks; ++i)
    {
        for (int k = 0; k < block_size; ++k)
        {
            sampling_blocks[i].push_back(i * block_size + k);
        }
    }

    return sampling_blocks;
}
}