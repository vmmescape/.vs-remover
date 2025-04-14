#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <windows.h>

#ifdef UNICODE
#undef UNICODE
#endif
#ifdef _UNICODE
#undef _UNICODE
#endif

std::string format_size( DWORD64 size ) 
{
    if ( size < 1024 ) return std::to_string( size ) + " B";
    else if ( size < 1048576 ) return std::to_string( size / 1024 ) + " KB";
    else if  (size < 1073741824 ) return std::to_string( size / 1048576 ) + " MB";
    else return std::to_string( size / 1073741824 ) + " GB";
}

std::string normalize_path( std::string path ) 
{
    std::transform( path.begin( ), path.end( ), path.begin(),[ ]( unsigned char c ) { return std::tolower( c ); } );

    std::replace( path.begin( ), path.end( ), '/', '\\' );

    if ( !path.empty( ) && path.back( ) != '\\' ) 
    {
        path += '\\';
    }

    return path;
}

bool excluded( const std::string& path, const std::vector<std::string>& exclusions ) 
{
    std::string normalized_path = normalize_path( path );

    for ( const auto& exclusion : exclusions ) 
    {
        if ( normalized_path.find( exclusion ) == 0 ) return true;
    }

    return false;
}

std::vector<std::string> read_excluded( const std::string& filename ) 
{
    std::vector<std::string> exclusions;
    std::ifstream file( filename );

    if ( file.is_open( ) ) 
    {
        std::string line;
        while (std::getline( file, line ) ) 
        {
            if (!line.empty( ) ) 
            {
                std::string normalized = normalize_path( line );
                exclusions.push_back( normalized );      
            }
        }
        file.close( );
    }
    else 
        std::cout << "Could not open exclusions file!" << std::endl;
    
    return exclusions;
}

void add_exclusion( const std::string& path, const std::string& filename ) 
{
    std::ofstream file( filename, std::ios::app );
    if ( file.is_open( ) )   
    {
        file << path << std::endl;
        file.close( );
        std::cout << "Added exclusion: " << path << std::endl;
    }
    else 
        std::cout << "Error: Could not open exclusions file." << std::endl;
    
}

DWORD64 calculate_dir_size( const std::string& dir_path ) 
{
    DWORD64 total_size = 0;
    WIN32_FIND_DATAA find_data;

    std::string search_path = dir_path + "\\*";
    HANDLE find_handle = FindFirstFileA( search_path.c_str( ), &find_data );

    if ( find_handle != INVALID_HANDLE_VALUE ) 
    {
        do 
        {
            std::string current_file = find_data.cFileName;

            if ( current_file == "." || current_file == ".." ) continue;

            std::string full_path = dir_path + "\\" + current_file;

            if ( find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) 
            {
                total_size += calculate_dir_size( full_path );
            }
            else 
            {
                DWORD64 file_size = ( static_cast<DWORD64>( find_data.nFileSizeHigh ) << 32 ) + find_data.nFileSizeLow;
                total_size += file_size;
            }
        } while ( FindNextFileA( find_handle, &find_data ) );

        FindClose( find_handle );
    }

    return total_size;
}

bool delete_directory( const std::string& dir_path ) 
{
    WIN32_FIND_DATAA find_data;
    std::string search_path = dir_path + "\\*";
    HANDLE find_handle = FindFirstFileA( search_path.c_str( ), &find_data );

    if ( find_handle == INVALID_HANDLE_VALUE ) return false;

    bool success = true;

    do 
    {
        std::string current_file = find_data.cFileName;

        if ( current_file == "." || current_file == "..") continue;
        
        std::string full_path = dir_path + "\\" + current_file;

        if ( find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
        {
            success = delete_directory( full_path ) && success;
        }
        else        
            if ( DeleteFileA( full_path.c_str( ) ) == 0 ) 
            {
                std::cout << "Failed to delete file: " << full_path << " ( Error: " << GetLastError( ) << " )" << std::endl;
                success = false;
            }
        
    } while ( FindNextFileA( find_handle, &find_data ) );

    FindClose( find_handle );

    if ( RemoveDirectoryA( dir_path.c_str( ) ) == 0 ) 
    {
        std::cout << "Failed to remove directory: " << dir_path << " ( Error: " << GetLastError( ) << ")" << std::endl;
        success = false;
    }

    return success;
}

void find_vs_directories( const std::string& search_path, const std::vector<std::string>& exclusions, bool test_run ) 
{
    size_t dir_count = 0;
    size_t excluded_count = 0;
    DWORD64 total_size = 0;
    DWORD64 excluded_size = 0;

    std::cout << "Searching for .vs directories in " << search_path << "..." << std::endl;
    std::cout << "Exclusions loaded: " << exclusions.size( ) << std::endl;

    std::vector<std::string> directories_to_process;
    directories_to_process.push_back( search_path );

    while ( !directories_to_process.empty( ) ) 
    {
        std::string current_dir = directories_to_process.back( );
        directories_to_process.pop_back( );

        WIN32_FIND_DATAA find_data;
        std::string search_pattern = current_dir + "\\*";
        HANDLE find_handle = FindFirstFileA( search_pattern.c_str( ), &find_data );

        if ( find_handle == INVALID_HANDLE_VALUE ) continue;

        do 
        {
            std::string file_name = find_data.cFileName;

            if ( file_name == "." || file_name == ".." ) continue;
            
            std::string full_path = current_dir + "\\" + file_name;

            if ( find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) 
            {
                std::string file_name_lower = file_name;
                std::transform( file_name_lower.begin( ), file_name_lower.end( ), file_name_lower.begin( ),[ ]( unsigned char c ) { return std::tolower( c ); } );

                if ( file_name_lower == ".vs" ) 
                {
                    DWORD64 dir_size = calculate_dir_size( full_path );

                    bool is_excluded = false;

                    if ( excluded ( current_dir, exclusions ) ) 
                    {
                        is_excluded = true;
                    }

                    if ( is_excluded ) 
                    {
                        excluded_count++;
                        excluded_size += dir_size;
                        std::cout << "EXCLUDED (will NOT be deleted): " << full_path << " (" << format_size(dir_size) << ")" << std::endl;
                    }
                    else {
                      
                        dir_count++;
                        total_size += dir_size;

                        std::cout << "WILL BE DELETED: " << full_path << " (" << format_size(dir_size) << ")" << std::endl;

                        if ( !test_run ) {
                            if ( delete_directory( full_path ) ) 
                            {
                                std::cout << "Deleted: " << full_path << std::endl;
                            }
                            else 
                            {
                                std::cout << "Failed to delete directory: " << full_path << std::endl;
                            }
                        }
                    }
                }
                else 
                {
                    directories_to_process.push_back( full_path );
                }
            }
       
        } while ( FindNextFileA( find_handle, &find_data ) );

        FindClose( find_handle );
    }

    std::cout << "\nSummary:" << std::endl;
    if ( test_run )
    {
        std::cout << "Found " << ( dir_count + excluded_count ) << " total .vs directories" << std::endl;
        std::cout << dir_count << " directories will be deleted (" << format_size( total_size ) << ")" << std::endl;
        std::cout << excluded_count << " directories will be preserved due to exclusions (" << format_size(excluded_size) << ")" << std::endl;
        std::cout << "This was a test run. No files were deleted." << std::endl;
    }   
    else 
        std::cout << "Deleted " << dir_count << " .vs directories (" << format_size( total_size ) << ")" << std::endl;
        std::cout << "Preserved " << excluded_count << " excluded directories (" << format_size( excluded_size ) << ")" << std::endl;
    
}

int main( ) 
{
    const std::string txt = "excluded.txt";
    const std::string search_root = "C:\\";

    while ( true ) {
        std::cout << "\n.VS Directory Cleaner" << std::endl;
        std::cout << "--------------------" << std::endl;
        std::cout << "1) Clean .vs directories" << std::endl;
        std::cout << "2) Add exclusion path" << std::endl;
        std::cout << "3) View exclusions" << std::endl;
        std::cout << "4) Test run (show what would be deleted)" << std::endl;
        std::cout << "5) Exit" << std::endl;
        std::cout << "Select an option: ";

        int choice;
        std::cin >> choice;
        std::cin.ignore( );

        auto exclusions = read_excluded( txt );

        switch ( choice ) 
        {
        case 1: 
        {
            std::cout << "\nCleaning .vs directories" << std::endl;

            find_vs_directories( search_root, exclusions, false );
            break;
        }
        case 2: 
        {
            std::string path;
            std::cout << "Enter path to exclude: ";
            std::getline( std::cin, path );

            add_exclusion( path, txt );
            break;
        }
        case 3: 
        {
            std::cout << "\nCurrent exclusions:" << std::endl;

            if ( exclusions.empty( ) ) 
            {
                std::cout << "No exclusions found." << std::endl;
            }
            else 
                for ( size_t i = 0; i < exclusions.size( ); i++) 
                {
                    std::string display = exclusions[ i ];
                    if ( !display.empty( ) && display.back( ) == '\\' ) 
                    {
                        display.pop_back( ); 
                    }
                    std::cout << i + 1 << ": " << display << std::endl;
                }
            
            break;
        }
        case 4: 
        {
            std::cout << "\nPerforming test run" << std::endl;
            find_vs_directories( search_root, exclusions, true );
            break;
        }
        case 5:
        {
            std::cout << "Exiting program." << std::endl;
            return 0;
        }
        default:
            std::cout << "Invalid option. Please try again." << std::endl;
        }
    }

    return 0;
}