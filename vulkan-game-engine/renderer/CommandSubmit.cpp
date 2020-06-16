#include "CommandSubmit.hpp"

#include <renderer/GraphicEngineConstants.hpp>
#include <renderer/GameGraphicEngine.hpp>
#include <renderer/RenderQuery.hpp>

#include <tools/ResultHandler.hpp>

noxcain::CommandSubmit::CommandSubmit() : submit_thread( &CommandSubmit::submit_loop, this )
{
	free_buffer_ids.resize( RECORD_RING_SIZE );
	for( std::size_t index = 0; index < RECORD_RING_SIZE; ++index )
	{
		free_buffer_ids[index] = index;
	}
}

noxcain::CommandSubmit::~CommandSubmit()
{
	{
		std::unique_lock lock( submit_mutex );
		shutdown = true;
		submit_condition.notify_all();
	}

	if( submit_thread.joinable() )
	{
		submit_thread.join();
	}

	vk::Device device = GraphicEngine::get_device();
	if( device )
	{
		ResultHandler r_handler( vk::Result::eSuccess );
		r_handler << device.waitIdle();
		if( r_handler.all_okay() )
		{
			device.destroyFence( frame_end_fence );
			for( const auto& semaphore : semaphores )
			{
				device.destroySemaphore( semaphore );
			}
		}
	}
}

noxcain::INT32 noxcain::CommandSubmit::get_free_buffer_id()
{
	std::unique_lock lock( submit_mutex );
	submit_condition.wait( lock, [this]() { return !free_buffer_ids.empty() || shutdown; } );
	if( shutdown )
	{
		return -1;
	}
	
	INT32 id( free_buffer_ids.back() );
	free_buffer_ids.pop_back();
	return id;
}

bool noxcain::CommandSubmit::set_newest_command_buffer( SubmitCommandBufferData buffer_data )
{
	std::unique_lock lock( submit_mutex );
	if( !shutdown )
	{
		// ignore buffer when swapchain was recreated because it would be outdated
		if( swapchain_is_new )
		{
			free_buffer_ids.push_back( buffer_data.id );
			swapchain_is_new = false;
			buffer_available = false;
			return true;
		}
		
		// last buffer was not used ( graphic card to slow ) --> give it free
		if( buffer_available )
		{
			free_buffer_ids.push_back( available_buffer.id );
			submit_condition.notify_all();
		}
		// set new buffer
		else
		{
			buffer_available = true;
		}
		available_buffer = buffer_data;
		return true;
	}
	return false;
}

noxcain::UINT32 noxcain::CommandSubmit::submit_loop()
{
	Watcher watcher( shutdown, submit_mutex, submit_condition );
	ResultHandler<vk::Result> r_handler( vk::Result::eSuccess );

	const vk::Device device = GraphicEngine::get_device();
	const vk::Queue& queue = device.getQueue( GraphicEngine::get_graphic_queue_family_index(), 0 );
	
	frame_end_fence = r_handler << device.createFence( vk::FenceCreateInfo( vk::FenceCreateFlags() ) );
	for( std::size_t index = 0; index < SEMOPHORE_COUNT; ++index )
	{
		semaphores[index] = r_handler << device.createSemaphore( vk::SemaphoreCreateInfo( vk::SemaphoreCreateFlags() ) );
	}
	
	if( !r_handler.all_okay() ) return 1;

	while( !shutdown )
	{
		// get command buffers
		SubmitCommandBufferData current_buffers;
		{
			std::unique_lock lock( submit_mutex );
			submit_condition.wait( lock, [this] { return buffer_available || shutdown; } );
			current_buffers = available_buffer;
			buffer_available = false;
		}

		time_collection_all.start_frame( 0.0, 0.7, 0.3, 1.0, "" );

		// submit command buffers
		{
			const vk::SwapchainKHR& swapChain = GraphicEngine::get_swapchain();
			UINT32 image_index = r_handler << device.acquireNextImageKHR( swapChain, GRAPHIC_TIMEOUT_DURATION.count(), get_semaphore( SemaphoreIds::ACQUIRE ), vk::Fence() );

			if( r_handler.all_okay() )
			{	
				vk::PipelineStageFlags render_stage_flags( vk::PipelineStageFlagBits::eColorAttachmentOutput );
				vk::PipelineStageFlags sampling_stage_flags( vk::PipelineStageFlagBits::eFragmentShader );
				
				r_handler << queue.submit(
					{
						vk::SubmitInfo(
							1, &get_semaphore( SemaphoreIds::ACQUIRE ), &render_stage_flags,
							1, &( current_buffers.main_buffer ),
							1, &get_semaphore( SemaphoreIds::RENDER ) ),
						vk::SubmitInfo(
							1, &get_semaphore( SemaphoreIds::RENDER ), &sampling_stage_flags,
							1, &( current_buffers.finalize_command_buffers[image_index] ),
							1, &get_semaphore( SemaphoreIds::SUPER_SAMPLING ) )
					},
					frame_end_fence );

				if( r_handler.all_okay() )
				{
					r_handler << queue.presentKHR( vk::PresentInfoKHR( 1, &get_semaphore( SemaphoreIds::SUPER_SAMPLING ), 1, &swapChain, &image_index ) );
					if( r_handler.all_okay() )
					{
						r_handler << device.waitForFences( { frame_end_fence }, VK_TRUE, ~UINT64(0) );

						if( r_handler.all_okay() )
						{
							std::array<UINT64, RenderQuery::TIMESTAMP_COUNT> time_stamps;
							auto end_time = std::chrono::steady_clock::now();
							auto timestamp_pool = GraphicEngine::get_render_query().get_timestamp_pool();
							if( timestamp_pool )
							{
								DOUBLE timestamp_period = GraphicEngine::get_physical_device().getProperties().limits.timestampPeriod;
								/*r_handler <<*/ device.getQueryPoolResults( timestamp_pool, 0, RenderQuery::TIMESTAMP_COUNT, vk::ArrayProxy<UINT64>( time_stamps ), 0, vk::QueryResultFlagBits::e64 );
								if( r_handler.all_okay() )
								{
									time_collection_gpu.add_time_frame(
										end_time - std::chrono::nanoseconds( UINT64( timestamp_period * ( time_stamps[(UINT32)RenderQuery::TimeStampIds::END] - time_stamps[(UINT32)RenderQuery::TimeStampIds::START] ) ) ),
										end_time - std::chrono::nanoseconds( UINT64( timestamp_period * ( time_stamps[(UINT32)RenderQuery::TimeStampIds::END] - time_stamps[(UINT32)RenderQuery::TimeStampIds::AFTER_GEOMETRY] ) ) ),
										0.8, 0.8, 0.0, 0.8, "geometry" );

									time_collection_gpu.add_time_frame(
										end_time - std::chrono::nanoseconds( UINT64( timestamp_period * ( time_stamps[(UINT32)RenderQuery::TimeStampIds::END] - time_stamps[(UINT32)RenderQuery::TimeStampIds::AFTER_GEOMETRY] ) ) ),
										end_time - std::chrono::nanoseconds( UINT64( timestamp_period * ( time_stamps[(UINT32)RenderQuery::TimeStampIds::END] - time_stamps[(UINT32)RenderQuery::TimeStampIds::AFTER_GLYPHS] ) ) ),
										0.8, 0.0, 0.0, 0.8, "vector" );

									time_collection_gpu.add_time_frame(
										end_time - std::chrono::nanoseconds( UINT64( timestamp_period * ( time_stamps[(UINT32)RenderQuery::TimeStampIds::END] - time_stamps[(UINT32)RenderQuery::TimeStampIds::AFTER_GLYPHS] ) ) ),
										end_time - std::chrono::nanoseconds( UINT64( timestamp_period * ( time_stamps[(UINT32)RenderQuery::TimeStampIds::END] - time_stamps[(UINT32)RenderQuery::TimeStampIds::AFTER_SHADING] ) ) ),
										0.8, 0.0, 0.8, 0.8, "shading" );

									time_collection_gpu.add_time_frame(
										end_time - std::chrono::nanoseconds( UINT64( timestamp_period * ( time_stamps[(UINT32)RenderQuery::TimeStampIds::END] - time_stamps[(UINT32)RenderQuery::TimeStampIds::BEFOR_POST] ) ) ),
										end_time - std::chrono::nanoseconds( UINT64( timestamp_period * ( time_stamps[(UINT32)RenderQuery::TimeStampIds::END] - time_stamps[(UINT32)RenderQuery::TimeStampIds::AFTER_POST] ) ) ),
										0.0, 0.0, 0.8, 0.8, "post" );

									time_collection_gpu.add_time_frame(
										end_time - std::chrono::nanoseconds( UINT64( timestamp_period * ( time_stamps[(UINT32)RenderQuery::TimeStampIds::END] - time_stamps[(UINT32)RenderQuery::TimeStampIds::BEFOR_OVERLAY] ) ) ),
										end_time - std::chrono::nanoseconds( UINT64( timestamp_period * ( time_stamps[(UINT32)RenderQuery::TimeStampIds::END] - time_stamps[(UINT32)RenderQuery::TimeStampIds::AFTER_OVERLAY] ) ) ),
										0.0, 0.8, 0.8, 0.8, "overlay" );
								}
							}
							r_handler << device.resetFences( { frame_end_fence } );
							if( !r_handler.all_okay() ) return 1;
						}
					}
				}

			}
		}

		// free command buffers
		{
			std::unique_lock lock( submit_mutex );
			free_buffer_ids.push_back( current_buffers.id );
			submit_condition.notify_all();
		}
		time_collection_all.end_frame();
	}
	return 0;
}