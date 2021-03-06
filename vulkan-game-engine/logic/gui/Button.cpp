#include "Button.hpp"

void noxcain::BaseButton::activate()
{
	background.set_color( normal_color );
	if( !active )
	{
		text_content.get_text().set_color( active_text_color );
	}
	RegionalEventRecieverNode::activate();
}

void noxcain::BaseButton::deactivate()
{
	background.set_color( inactive_color );
	active_text_color = text_content.get_text().get_color();
	text_content.get_text().set_color( inactive_text_color );
	RegionalEventRecieverNode::deactivate();
}

void noxcain::BaseButton::hide()
{
	VectorTextLabel2D::hide();
	deactivate();
}

void noxcain::BaseButton::show()
{
	VectorTextLabel2D::show();
	activate();
}

bool noxcain::BaseButton::try_hit( const RegionalKeyEvent& regional_event ) const
{
	return frame.check_region( regional_event.get_x_position(), regional_event.get_y_position() );
}

bool noxcain::BaseButton::hit( const RegionalKeyEvent& key_event )
{
	if( !was_hitted )
	{
		if( enter_handler && !enter_handler( key_event, *this ) )
		{
			return false;
		}
		background.set_color( over_color );
	}

	if( over_handler )
	{
		over_handler( key_event, *this );
	}

	if( key_event.get_event() == RegionalKeyEvent::Events::DOWN )
	{
		if( !down_handler || down_handler( key_event, *this ) )
		{
			background.set_color( click_color );
			last_key_code = key_event.get_key_code();
		}
	}
	else if( key_event.get_event() == RegionalKeyEvent::Events::UP )
	{
		if( !up_handler || up_handler( key_event, *this ) )
		{
			background.set_color( over_color );
			if( click_handler &&
				last_key_code != RegionalKeyEvent::KeyCodes::NONE &&
				last_key_code == key_event.get_key_code() )
			{
				click_handler( key_event, *this );
			}
		}
	}
	return true;
}

bool noxcain::BaseButton::miss( const RegionalKeyEvent& key_event )
{
	if( was_hitted )
	{
		if( leave_handler && !leave_handler( key_event, *this ) )
		{
			return false;
		}
		background.set_color( normal_color );
	}
	last_key_code = RegionalKeyEvent::KeyCodes::NONE;
	return true;
}

noxcain::BaseButton::BaseButton( GameUserInterface& ui ) : VectorTextLabel2D( ui )
{
	set_frame_size( 1 );
	set_frame_color( 0.6, 0.6, 0.6 );
	set_background_color( 0.9, 0.9, 0.9 );
	set_active_color( 0.6, 0.6, 0.6 );
	set_highlight_color( 1.0, 1.0, 1.0 );
	get_text_element().set_color( 0.2, 0.2, 0.2 );
}
