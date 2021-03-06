#pragma once

#include <Defines.hpp>

#include <array>

#include <logic/Region.hpp>
#include <logic/Renderable.hpp>
#include <logic/VectorText.hpp>

#include <vulkan/vulkan.hpp>

namespace noxcain
{
	class VectorText2D : public Renderable<VectorText2D>, public Region
	{
	public:
		VectorText2D( Renderable<VectorText2D>::List& visibility_list );

		const VectorText& get_text() const
		{
			return text;
		}

		VectorText& get_text()
		{
			return text;
		}

		void set_scissor( const Region* scissor_ )
		{
			scissor = scissor_;
		}

		const Region* get_scissor() const
		{
			return scissor;
		}

		void set_depth_level( UINT32 depth_ )
		{
			depth = depth_;
		}

		UINT32 get_depth_level() const
		{
			return depth;
		}

		vk::Rect2D record( const vk::CommandBuffer& command_buffer, vk::PipelineLayout pipeline_layout, vk::Rect2D current_scissor, vk::Rect2D default_scissor ) const;
	private:
		UINT32 depth = 0;
		VectorText text;
		const Region* scissor = nullptr;
	};
}