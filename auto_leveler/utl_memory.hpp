#pragma once
#include <windows.h>
#include <TlHelp32.h>
#include <Psapi.h>

#include <string_view>
#include <sstream>
#include <array>
#include <algorithm>
#include <vector>
#include <iostream>

namespace process
{
	namespace detail
	{
		using uq_handle = std::unique_ptr<void, decltype( &CloseHandle )>;

		uq_handle _process_handle = { nullptr, nullptr };
		uint64_t _base_address = 0;

		enum pattern_type
		{
			pattern_raw = 0,
			pattern_mov
		};

		std::pair<std::vector<uint8_t>, std::string> ida_to_bytes( const std::string_view signature )
		{
			std::vector<std::uint8_t> buffer{};

			// now use std::getline to seperate the string into tokens based on spaces
			std::vector<std::string> total_tokens{};
			std::string curr_token{};
			std::istringstream str_stream{ signature.data( ) };

			while ( std::getline( str_stream, curr_token, ' ' ) )
				total_tokens.push_back( curr_token );

			buffer.resize( total_tokens.size( ) );

			// convert the newly seperated string into bytes (wildcards will just be zero)
			std::transform( total_tokens.cbegin( ), total_tokens.cend( ), buffer.begin( ), [ ]( const std::string& curr_token ) -> uint8_t
							{
								return curr_token.find( '?' ) == std::string::npos ? std::stoi( curr_token, nullptr, 16 ) : 0;
							} );

			// since im too lazy to handle this in the other function, just make a mask using pretty much the same method.
			std::string signature_mask{};
			signature_mask.resize( buffer.size( ) );

			std::transform( buffer.cbegin( ), buffer.cend( ), signature_mask.begin( ), [ ]( const uint8_t curr_byte ) -> uint8_t
							{
								return curr_byte == 0 ? '?' : 'x';
							} );

			return { buffer, signature_mask };
		}

		bool pattern_scan_helper( const uint8_t* data, const uint8_t* signature, const char* mask )
		{
			for ( ; *mask; ++mask, ++data, ++signature )
				if ( *mask == 'x' && *data != *signature )
					return false;

			return ( *mask ) == 0;
		}
	}

	bool initialize( const std::wstring_view process_name )
	{
		const auto snap_shot = detail::uq_handle{ CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 ), &CloseHandle };

		if ( !snap_shot )
		{
			std::cout << "[!] can't obtain process list snapshot\n";
			return false;
		}

		PROCESSENTRY32W entry{ sizeof( PROCESSENTRY32W ) };

		for ( Process32FirstW( snap_shot.get( ), &entry ); Process32NextW( snap_shot.get( ), &entry ); )
		{
			if ( std::wcscmp( entry.szExeFile, process_name.data( ) ) != 0 )
				continue;

			detail::_process_handle = detail::uq_handle{ OpenProcess( PROCESS_ALL_ACCESS, FALSE, entry.th32ProcessID ), &CloseHandle };
			break;
		}

		if ( !detail::_process_handle )
		{
			std::cout << "[!] can't obtain process handle, error: 0x" << std::hex << GetLastError( ) << '\n';
			return false;
		}

		std::cout << "[+] process handle: 0x" << std::hex << detail::_process_handle.get( ) << '\n';

		auto loaded_modules = std::make_unique<HMODULE[ ]>( 64 );
		DWORD loaded_module_sz = 0;

		if ( !EnumProcessModules( detail::_process_handle.get( ), loaded_modules.get( ), 512, &loaded_module_sz ) )
		{
			std::cout << "[!] can't obtain process modules, error: 0x" << std::uppercase << std::hex << GetLastError( ) << '\n';
			return {};
		}

		for ( auto i = 0u; i < loaded_module_sz / 8u; i++ )
		{
			wchar_t file_name[ MAX_PATH ] = L"";

			if ( !GetModuleFileNameExW( detail::_process_handle.get( ), loaded_modules[ i ], file_name, _countof( file_name ) ) )
				continue;

			if ( std::wcsstr( file_name, process_name.data( ) ) == nullptr )
				continue;

			detail::_base_address = reinterpret_cast< uint64_t >( loaded_modules[ i ] );
			break;
		}

		std::cout << "[+] process base: 0x" << std::uppercase << std::hex << detail::_base_address << '\n';

		return true;
	}

	bool read_vmem( uint64_t address, void* buffer, size_t size )
	{
		return ReadProcessMemory( detail::_process_handle.get( ), reinterpret_cast< void* >( address ), buffer, size, nullptr );
	}

	template <typename T = uint64_t>
	T read_vmem( uint64_t address )
	{
		T buffer{};
		read_vmem( address, &buffer, sizeof( T ) );
		return buffer;
	}

	bool write_vmem( uint64_t address, void* buffer, size_t size )
	{
		return WriteProcessMemory( detail::_process_handle.get( ), reinterpret_cast< void* >( address ), buffer, size, nullptr );
	}

	template <typename T>
	bool write_vmem( uint64_t address, T data )
	{
		return write_vmem( address, &data, sizeof( T ) );
	}

	template <detail::pattern_type type = detail::pattern_mov>
	uint64_t pattern_scan( const std::string_view signature )
	{
		const auto ida_signature = detail::ida_to_bytes( signature );

		const auto uq_buffer = std::make_unique<std::array<uint8_t, 0x100000ull>>( );
		const auto buffer_data = uq_buffer->data( );

		//
		// :)
		//
		for ( auto i = 0ull; i < 0x4000000ull; i++ )
		{
			if ( !read_vmem( detail::_base_address + i * 0x100000ull, buffer_data, 0x100000ull ) || !buffer_data )
				continue;

			for ( auto j = 0ull; j < 0x100000ull; j++ )
			{
				if ( !detail::pattern_scan_helper( buffer_data + j, reinterpret_cast< const uint8_t* >( ida_signature.first.data( ) ), ida_signature.second.c_str( ) ) )
					continue;

				auto result = detail::_base_address + i * 0x100000ull + j;

				switch ( type )
				{
				case detail::pattern_raw:
				//
				// return the raw result, without resolving address or anything.
				//
				break;
				case detail::pattern_mov:
				{
					//
					// mov is structured like:
					// mov register, source
					// 48 RR RR (source)
					// read rip-relative source (+ 3, size of 4 bytes), add it to current rip then add size of the instruction (7)
					//
					result = read_vmem( result + read_vmem<int32_t>( result + 3 ) + 7 );
					break;
				}
				default:break;
				}

				return result;
			}
		}

		return 0;
	}
}