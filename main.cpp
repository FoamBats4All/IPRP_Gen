#include "StdAfx.h"

typedef std::vector<unsigned long> ULongVect;

// Data types for surfing cached 2DA data.
std::string Get2DATlkString( ResourceManager& resources, std::string s2DAFile, unsigned int row, std::string column ) {
	unsigned long index;
	std::string result;

	resources.Get2DAUlong( s2DAFile, column, row, index );
	resources.GetTalkString( index, result );

	return result;
}

// Entry point.
int main( int argc, char** argv ) {
	// Allocate the reader.
	PrintfTextOut TextOut;

	// ...
	try {
		// Read the ini file.
		boost::program_options::options_description ini_desc;
		ini_desc.add_options()
			( "settings.module", "Name of the module to load." )
			( "paths.nwn2-install", "Path that NWN2 is installed in." )
			( "paths.nwn2-home", "Path where NWN2 user files are stored." );
		std::ifstream ini_file( "iprp_gen.ini" );
		boost::program_options::variables_map ini;
		boost::program_options::store( boost::program_options::parse_config_file( ini_file, ini_desc, true ), ini );

		// Get the check module.
		if ( !ini.count( "settings.module" ) ) throw std::exception( "Module not set." );
		std::string ModuleName = ini["settings.module"].as<std::string>();

		// Get the NWN2 install location.
		if ( !ini.count( "paths.nwn2-install" ) ) throw std::exception( "NWN2 install location not set." );
		std::string PathNWN2Install = ini["paths.nwn2-install"].as<std::string>();
		if ( !boost::filesystem::exists( PathNWN2Install ) ) throw std::exception( "NWN2 install location does not exist." );

		// Get the NWN2 home location.
		if ( !ini.count( "paths.nwn2-home" ) ) throw std::exception( "NWN2 home location not set." );
		std::string PathNWN2Home = ini["paths.nwn2-home"].as<std::string>();
		if ( !boost::filesystem::exists( PathNWN2Home ) ) throw std::exception( "NWN2 home location does not exist." );

		// Load the NWN2 Resource Manager and module.
		TextOut.WriteText( "Loading resources and module ..." );
		ResourceManager resources( &TextOut );
		LoadModule( resources, ModuleName.c_str(), PathNWN2Home.c_str(), PathNWN2Install.c_str() );
		std::cout << std::endl;

		// Build the spell ID list.
		unsigned long spellCount = resources.Get2DARowCount( "spells" );
		ULongVect SpellList;
		for ( unsigned long i = 0; i < spellCount; i++ ) {
			// Removed?
			bool removed;
			resources.Get2DABool( "spells", "REMOVED", i, removed );
			if ( removed ) continue;
			
			// Name valid?
			std::string Name;
			resources.Get2DAString( "spells", "Name", i, Name );
			if ( Name == "****" ) continue;
			
			// Not a feat?
			std::string FeatID;
			resources.Get2DAString( "spells", "FeatID", i, FeatID );
			if ( FeatID != "****" ) continue;

			// Valid innate?
			std::string Innate;
			resources.Get2DAString( "spells", "Innate", i, Innate );
			if ( Innate == "****" ) {
				std::cout << "WARNING: Spell ID #" << i << " (\"" << Get2DATlkString( resources, "spells", i, "Name" ) << "\") has no innate level." << std::endl;
				continue;
			}

			// All good, add.
			SpellList.push_back( i );
		}

		// Remove spell IDs from the list if they already have entries.
		std::cout << "Finding missing iprp_spells spells..." << std::endl;
		unsigned int IPRPSpellsCount = resources.Get2DARowCount( "iprp_spells" );
		for ( unsigned long i = 0; i < IPRPSpellsCount; i++ ) {
			unsigned long value;
			resources.Get2DAUlong( "iprp_spells", "SpellIndex", i, value );
			ULongVect::iterator needle = std::find( SpellList.begin(), SpellList.end(), value );
			if ( needle != SpellList.end() ) {
				SpellList.erase( needle );
			}
		}
		std::cout << "Found " << SpellList.size() << " missing spells." << std::endl;

		// Go through and add new content.
		std::cout << "Writing new rows." << std::endl;
		std::ofstream out( "iprp_spells-ADDITION.txt" );
		for ( unsigned long i = 0; i < SpellList.size(); i++ ) {
			// Row data.
			std::string Label, Name, Icon;
			unsigned long CasterLvl, InnateLvl, Cost, SpellIndex, PotionUse, WandUse, GeneralUse;
			
			// Pull data from spells.2da
			SpellIndex = SpellList[i];
			resources.Get2DAString( "spells", "Label", SpellIndex, Label );
			resources.Get2DAString( "spells", "Name", SpellIndex, Name );
			resources.Get2DAString( "spells", "IconResRef", SpellIndex, Icon );
			resources.Get2DAUlong( "spells", "Innate", SpellIndex, InnateLvl );

			// Get use states.
			GeneralUse = 1;
			PotionUse = ( InnateLvl <= 3 ) ? 1 : 0;
			WandUse = ( InnateLvl <= 4 ) ? 1 : 0;

			// Calculate CasterLevel cost.
			CasterLvl = InnateLvl*2 - 1;
			if ( InnateLvl == 0 ) CasterLvl = 0; 
			Cost = InnateLvl * CasterLvl * 360;
			
			// Write line.
			out << IPRPSpellsCount << " " << Label << "_(" << CasterLvl << ") " << Name << " " << CasterLvl;
			out << " " << InnateLvl << " " << Cost << " " << SpellIndex << " " << PotionUse << " " << WandUse;
			out << " " << GeneralUse << " " << Icon << "\n";
			IPRPSpellsCount++;
		}
		out.close();
		std::cout << "Process complete. Wrote " << SpellList.size() << " new rows." << std::endl;

	} catch ( std::exception &e ) {
		TextOut.WriteText( "\nError: %s\n", e.what() );
		system( "PAUSE" );
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}