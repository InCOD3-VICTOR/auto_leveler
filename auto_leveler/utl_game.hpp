#pragma once
#include <iostream>
#include "utl_memory.hpp"
#include "utl_variables.hpp"

namespace game
{
	namespace detail
	{
		bool initialize( )
		{
			// game manager virtual table, function at 0x138
			game::profile_manager = process::pattern_scan( "48 8b 05 ? ? ? ? 33 d2 4c 8b 40 78" );

			if ( !game::profile_manager )
			{
				std::cout << "[!] profile manager is outdated\n";
				return false;
			}

			std::cout << "[+] profile manager: 0x" << std::hex << game::profile_manager << '\n';

			// search for cmp [rcx+2e8h], 02
			game::round_manager = process::pattern_scan( "48 8b 05 ? ? ? ? 8b 90 e8 02" );

			if ( !game::round_manager )
			{
				std::cout << "[!] round manager is outdated\n";
				return false;
			}

			std::cout << "[+] round manager: 0x" << std::hex << game::round_manager << '\n';

			return true;
		}
	}

	// the game does a sanity check on all pointers, where it's held as a pointer to a pointer then they check the bitness of it + a variable in local storage.
	uint64_t get_profile( )
	{
		const auto player_profile_ptr = process::read_vmem( game::profile_manager + 0x68 );

		if ( !player_profile_ptr )
			return 0;

		return process::read_vmem( player_profile_ptr );
	}

	bool in_match( )
	{
		const auto game_state = process::read_vmem<int8_t>( round_manager + 0x2e8 );

		return game_state == 2 || game_state == 3;
	}
}