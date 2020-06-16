#include "DebugLevel.hpp"

#include <logic/InputEventHandler.hpp>
#include <logic/VectorText2D.hpp>

#include <logic/gui/ScrollBar.hpp>
#include <logic/gui/Button.hpp>

#include <tools/TimeFrame.hpp>

void noxcain::DebugLevel::initialize()
{
	setup_events();

	background = std::make_unique<PassivColorLabel>( label_list );
	background->set_top_anchor( get_screen_root(), -LABEL_DISTANCE );
	background->set_left_anchor( get_screen_root(), LABEL_DISTANCE );
	background->set_right_anchor( get_screen_root(), -LABEL_DISTANCE );
	background->set_depth_level( 0 );
	background->set_color( 0.5, 0.5, 0.5, 0.5 );

	scissor_label = std::make_unique<Region>();
	scissor_label->set_top_anchor( *background, -LABEL_DISTANCE );
	scissor_label->set_right_anchor( *background, -LABEL_DISTANCE );

	background->set_bottom_anchor( *scissor_label, -LABEL_DISTANCE );

	const auto& time_frame_collections = TimeFrameCollector::get_time_frames();

	time_frame_group_labels.reserve( time_frame_collections.size() );

	constexpr DOUBLE LABEL_WIDTH = 160.0;
	constexpr DOUBLE LABEL_HEIGHT = 25.0;
	constexpr DOUBLE BLOCK_HEIGHT = LABEL_HEIGHT + LABEL_DISTANCE;

	std::size_t index;
	for( index = 0; index < time_frame_collections.size(); ++index )
	{
		auto& group_description = index < time_frame_group_labels.size() ? time_frame_group_labels[index] : time_frame_group_labels.emplace_back( std::make_unique<TimeFrameLabel>( *this ) );
		group_description->setup_as_group_label( *background, LABEL_DISTANCE, -LABEL_DISTANCE - BLOCK_HEIGHT * index, LABEL_WIDTH, 25.0, BASE_COLOR_FAC, BASE_COLOR_FAC, BASE_COLOR_FAC, time_frame_collections[index]->get_description() );
	}

	// return button
	auto key_filter = []( const RegionalKeyEvent& key_event, BaseButton& reciever ) -> bool
	{
		return key_event.get_key_code() == RegionalKeyEvent::KeyCodes::LEFT_MOUSE;
	};
	
	exit_button->set_background_color( BASE_COLOR_FAC, BASE_COLOR_FAC, BASE_COLOR_FAC, 1.0 );
	exit_button->set_highlight_color( BASE_COLOR_FAC + 0.2, BASE_COLOR_FAC + 0.2, BASE_COLOR_FAC + 0.2, 1.0 );
	exit_button->set_active_color( BASE_COLOR_FAC + 0.4, BASE_COLOR_FAC + 0.4, BASE_COLOR_FAC + 0.4, 1.0 );

	exit_button->get_area().set_left_anchor( *background, LABEL_DISTANCE );
	exit_button->get_area().set_top_anchor( *background, -LABEL_DISTANCE - BLOCK_HEIGHT * index );
	exit_button->get_area().set_height( LABEL_HEIGHT );
	exit_button->get_area().set_width( LABEL_WIDTH );

	exit_button->set_down_handler( key_filter );
	exit_button->set_up_handler( key_filter );
	exit_button->show();

	exit_button->get_text_element().set_utf8( "Exit" );


	exit_button->set_click_handler( [this]( const RegionalKeyEvent& key_event, BaseButton& reciever ) -> bool
	{
		switch_off();
		return true;
	} );

	scissor_label->set_horizontal_anchor( HorizontalAnchorType::LEFT, *exit_button, HorizontalAnchorType::RIGHT, LABEL_DISTANCE );
	scissor_label->set_bottom_anchor( *exit_button );

	//scroll bar
	scroll_bar = std::make_unique<HorizontalScrollBar>( 1.0, label_list, text_list, get_exclusiv_handler() );
	scroll_bar->set_top( *exit_button );
	scroll_bar->set_bottom( *exit_button );
	scroll_bar->set_left( *scissor_label );
	scroll_bar->set_right( *scissor_label );
	scroll_bar->set_bar_color( 0.8, 0.8, 0.8 );
	scroll_bar->set_slider_color( BASE_COLOR_FAC, BASE_COLOR_FAC, BASE_COLOR_FAC );
	scroll_bar->set_slider_highlight_color( BASE_COLOR_FAC + 0.2, BASE_COLOR_FAC + 0.2, BASE_COLOR_FAC + 0.2 );
	scroll_bar->set_slider_drag_color( BASE_COLOR_FAC + 0.4, BASE_COLOR_FAC + 0.4, BASE_COLOR_FAC + 0.4 );
	scroll_bar->set_depth_level( 5 );
	scroll_bar->make_scalable( 0.025 );
	
	scroll_bar->set_slider_size( 0.05 );
	scroll_bar->set_position( 1.0 );
	scroll_bar->show();

	get_screen_root().add_branch( *background );
	background->add_branch( *exit_button );
	background->add_branch( *scroll_bar );

	for( ++index; index < time_frame_group_labels.size(); ++index )
	{
		time_frame_group_labels[index]->hide();
	}
	initialized = true;
}

void noxcain::DebugLevel::setup_events()
{
	key_handlers.resize( KEY_EVENT_COUNT );
	key_handlers[static_cast<std::size_t>( KeyEvents::EXIT )].set_key( 0, 0x1B );
}

void noxcain::DebugLevel::check_events( const std::chrono::nanoseconds& delta )
{
	if( key_handlers[static_cast<std::size_t>( KeyEvents::EXIT )].got_pushed() )
	{
		switch_off();
	}
}

void noxcain::DebugLevel::update_level_logic( const std::chrono::nanoseconds& deltaTime )
{
	if( !initialized )
	{
		initialize();
	}

	check_events( deltaTime );

	if( scroll_bar )
	{

	}

	const auto& end = performance_time_stamp - ( 1.0-scroll_bar->get_position() ) * PERFORMANCE_TIME_FRAME;
	const auto& start_frame = end - scroll_bar->get_scale() * PERFORMANCE_TIME_FRAME;

	DOUBLE factor = ( scissor_label->get_width() ) / ( end - start_frame ).count();

	const auto& collections = TimeFrameCollector::get_time_frames();

	std::size_t index;
	std::size_t time_frame_count = 0;
	for( index = 0; index < collections.size(); ++index )
	{
		for( const auto& time_frame : *collections[index] )
		{
			if( time_frame.end <= start_frame ) continue;
			if( time_frame.start_frame >= end ) break;

			auto& time_label = time_frame_count < time_frame_labels.size() ? time_frame_labels[time_frame_count] : time_frame_labels.emplace_back( std::make_unique<TimeFrameLabel>( *this ) );

			time_label->setup_as_time_frame(
				*time_frame_group_labels[index], factor * ( time_frame.start_frame - start_frame ).count(), ( time_frame.end - time_frame.start_frame ).count()* factor,
				time_frame.color[0], time_frame.color[1], time_frame.color[2],
				time_frame.description, std::chrono::duration_cast<std::chrono::milliseconds>( time_frame.end - time_frame.start_frame ).count(),
				*scissor_label );

			++time_frame_count;
		}
	}

	for( ; time_frame_count < time_frame_labels.size(); ++time_frame_count )
	{
		time_frame_labels[time_frame_count]->hide();
	}
}

noxcain::DebugLevel::DebugLevel() : exit_button( std::make_unique<BaseButton>( text_list, label_list ) )
{
	vector_label_renderables.emplace_back( text_list );
	color_label_renderables.emplace_back( label_list );
}

noxcain::DebugLevel::~DebugLevel()
{
}

void noxcain::DebugLevel::switch_on()
{
	TimeFrameCollector::deactivate();
	std::unique_lock lock( visibilty_mutex );
	performance_time_stamp = std::chrono::steady_clock::now();
	is_on = true;
}

void noxcain::DebugLevel::switch_off()
{
	if( initialized )
	{
		scroll_bar->set_slider_size( 0.05 );
		scroll_bar->set_position( 1.0 );
	}
	TimeFrameCollector::activate();
	std::unique_lock lock( visibilty_mutex );
	is_on = false;
}

bool noxcain::DebugLevel::on() const
{
	std::unique_lock lock( visibilty_mutex );
	return is_on;
}

noxcain::DebugLevel::TimeFrameLabel::TimeFrameLabel( DebugLevel& level ) : 
	frame( level.label_list ), background( level.label_list ), text( level.text_list )
{
	background.set_same_region( frame, LABEL_FRAME_WIDTH, LABEL_FRAME_WIDTH );

	text.set_anchor( background, HorizontalAnchorType::LEFT, VerticalAnchorType::BOTTOM, HorizontalAnchorType::LEFT, VerticalAnchorType::BOTTOM, LABEL_FRAME_WIDTH, -LABEL_FRAME_WIDTH );
	
	frame.set_depth_level( 10 );
	background.set_depth_level( 100 );
}

void noxcain::DebugLevel::TimeFrameLabel::setup_as_group_label( const Region& anchor_parent, DOUBLE x_offset, DOUBLE y_offset, DOUBLE width, DOUBLE height,
																DOUBLE red, DOUBLE green, DOUBLE blue, const std::string& description )
{
	frame.set_anchor( anchor_parent, HorizontalAnchorType::LEFT, VerticalAnchorType::TOP, x_offset, y_offset );
	frame.set_size( width, height );

	frame.show();
	background.show();
	text.show();

	auto& text_element = text.get_text();
	text_element.set_color( 1.0, 1.0, 1.0, 1.0 );
	text_element.set_size( background.get_height() - 2*LABEL_FRAME_WIDTH );
	text_element.set_utf8( description );

	background.set_color( red, green, blue, 1.0 );
	frame.set_color( 0.8, 0.8, 0.8, 1.0 );
}

void noxcain::DebugLevel::TimeFrameLabel::hide()
{
	text.hide();
	background.hide();
	frame.hide();
}

noxcain::Region& noxcain::DebugLevel::TimeFrameLabel::get_region()
{
	return frame;
}

void noxcain::DebugLevel::TimeFrameLabel::set_color( DOUBLE red, DOUBLE green, DOUBLE blue )
{
	background.set_color( red, green, blue, 1.0 );
}

noxcain::DebugLevel::TimeFrameLabel::operator noxcain::Region& ( )
{
	return frame;
}

noxcain::DebugLevel::TimeFrameLabel::operator const noxcain::Region& ( ) const
{
	return frame;
}

void noxcain::DebugLevel::TimeFrameLabel::setup_as_time_frame( const Region& group_label,
															   DOUBLE start, DOUBLE width, 
															   DOUBLE red, DOUBLE green, DOUBLE blue,
															   const std::string& description, UINT32 milliseconds, 
															   const Region& scissor  )
{
	frame.set_top_anchor( group_label );
	frame.set_bottom_anchor( group_label );
	frame.set_left_anchor( scissor, start );
	frame.set_width( width );
	
	frame.show();
	background.show();
	text.show();

	text.set_scissor( &scissor );
	background.set_scissor( &scissor );
	frame.set_scissor( &scissor );

	auto& text_element = text.get_text();
	text_element.set_color( 1.0, 1.0, 1.0, 1.0 );
	text_element.set_size( background.get_height() - 2*LABEL_FRAME_WIDTH );
	const DOUBLE max_width = background.get_width() - 2*LABEL_FRAME_WIDTH;

	std::string descr = description;
	std::string time = std::to_string( milliseconds ) + " ms";
	text_element.set_utf8( descr.empty() ? time : descr + ": " + time );
	if( !descr.empty() && text.get_width() > max_width )
	{
		text_element.set_utf8( "..."+time );
		if( text.get_width() > max_width )
		{
			text.hide();
		}
	}
	background.set_color( red, green, blue, 1.0 );
	frame.set_color( 0.8F, 0.8F, 0.8F, 1.0F );
}