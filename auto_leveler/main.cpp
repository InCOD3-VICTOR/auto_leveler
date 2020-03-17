#include <thread>
#include <chrono>

#include "utl_player.hpp"
#include "utl_other.hpp"

#undef max

int main( )
{
	if ( !process::initialize( L"RainbowSix.exe" ) )
	{
		std::cin.ignore( );
		return 0;
	}

	if ( !game::detail::initialize( ) )
	{
		std::cin.ignore( );
		return 0;
	}

	std::cout << "[>] wanted level: ";

	int32_t wanted_level = 0;
	std::cin >> wanted_level;

	while ( wanted_level > player::get_level( ) )
	{
		static auto played_once = false;

		if ( !game::in_match( ) )
		{
			// simulate a left arrow key press (to switch to the retry situation button)
			// simulate an ENTER key press (to press the retry situation button instantly)
			if ( played_once )
			{
				input::send( VK_LEFT );
				input::send( VK_RETURN );
			}

			continue;
		}

		const auto game_profile = game::get_profile( );

		if ( !game_profile )
			continue;

		game::local_player = process::read_vmem( game_profile + 0x28 );

		player::set_score( std::numeric_limits<int32_t>::max( ) );
		player::set_health( 0 );
		player::switch_team( );

		played_once = true;

		std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );
	}

	std::cout << "[+] done\n";

	std::cin.clear( );
	std::cin.ignore( );
	return 0;
}