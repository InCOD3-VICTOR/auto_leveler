#pragma once
#include "utl_game.hpp"

// optimized for local player only
namespace player
{
	int32_t get_level( )
	{
		const auto player_profile = game::get_profile( );

		if ( !player_profile )
			return 0;

		return process::read_vmem<int32_t>( player_profile + 0x5c0 );
	}

	void switch_team( )
	{
		if ( !game::local_player )
			return;

		const auto player_info = process::read_vmem( game::local_player + 0xc8 );

		if ( !player_info )
			return;

		const auto mode_info = process::read_vmem( player_info + 0x88 );

		if ( !mode_info )
			return;

		const auto current_side = process::read_vmem<int8_t>( mode_info + 0x30 );
		process::write_vmem<int8_t>( mode_info + 0x30, current_side == 3 ? 4 : 3 );
	}

	void set_health( int32_t new_health )
	{
		if ( !game::local_player )
			return;

		const auto event_listener = process::read_vmem( game::local_player + 0x28 );

		if ( !event_listener )
			return;

		const auto player_events = process::read_vmem( event_listener + 0xd8 );

		if ( !player_events )
			return;

		const auto health_event = process::read_vmem( player_events + 0x8 );

		if ( !health_event )
			return;

		process::write_vmem<int32_t>( health_event + 0x168, new_health );
	}

	void set_score( int32_t new_score )
	{
		if ( !game::local_player )
			return;

		const auto player_info = process::read_vmem( game::local_player + 0xc8 );

		if ( !player_info )
			return;

		process::write_vmem<int32_t>( player_info + 0x154, new_score );
	}
}