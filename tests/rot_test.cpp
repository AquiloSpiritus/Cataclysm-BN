#include "catch/catch.hpp"

#include <memory>

#include "calendar.h"
#include "enums.h"
#include "item.h"
#include "map.h"
#include "map_helpers.h"
#include "point.h"
#include "weather.h"

static void set_map_temperature( int new_temperature )
{
    get_weather().temperature = new_temperature;
    get_weather().clear_temp_cache();
}

TEST_CASE( "Rate of rotting" )
{
    SECTION( "Passage of time" ) {
        // Item rot is a time duration.
        // At 65 F (18,3 C) item rots at rate of 1h/1h
        // So the level of rot should be about same as the item age
        // In preserving containers and in freezer the item should not rot at all

        // Items created at turn zero are handled differently, so ensure we're
        // not there.
        if( calendar::turn <= calendar::start_of_cataclysm ) {
            calendar::turn = calendar::start_of_cataclysm + 1_minutes;
        }

        item normal_item( "meat_cooked" );

        item freeze_item( "offal_canned" );

        item sealed_item( "offal_canned" );
        sealed_item = sealed_item.in_its_container();

        set_map_temperature( 65 ); // 18,3 C

        normal_item.process( nullptr, tripoint_zero, false, 1, temperature_flag::TEMP_NORMAL );
        sealed_item.process( nullptr, tripoint_zero, false, 1, temperature_flag::TEMP_NORMAL );
        freeze_item.process( nullptr, tripoint_zero, false, 1, temperature_flag::TEMP_NORMAL );

        // Item should exist with no rot when it is brand new
        CHECK( normal_item.get_rot() == 0_turns );
        CHECK( sealed_item.get_rot() == 0_turns );
        CHECK( freeze_item.get_rot() == 0_turns );

        INFO( "Initial turn: " << to_turn<int>( calendar::turn ) );

        calendar::turn += 20_minutes;
        normal_item.process( nullptr, tripoint_zero, false, 1, temperature_flag::TEMP_NORMAL );
        sealed_item.process( nullptr, tripoint_zero, false, 1, temperature_flag::TEMP_NORMAL );
        freeze_item.process( nullptr, tripoint_zero, false, 1, temperature_flag::TEMP_FREEZER );

        // After 20 minutes the normal item should have 20 minutes of rot
        CHECK( to_turns<int>( normal_item.get_rot() )
               == Approx( to_turns<int>( 20_minutes ) ).epsilon( 0.01 ) );
        // Item in freezer and in preserving container should have no rot
        CHECK( sealed_item.get_rot() == 0_turns );
        CHECK( freeze_item.get_rot() == 0_turns );

        // Move time 110 minutes
        calendar::turn += 110_minutes;
        sealed_item.process( nullptr, tripoint_zero, false, 1, temperature_flag::TEMP_NORMAL );
        freeze_item.process( nullptr, tripoint_zero, false, 1, temperature_flag::TEMP_FREEZER );
        // In freezer and in preserving container still should be no rot
        CHECK( sealed_item.get_rot() == 0_turns );
        CHECK( freeze_item.get_rot() == 0_turns );
    }
}

TEST_CASE( "Items rot away" )
{
    SECTION( "Item in reality bubble rots away" ) {
        // Item should rot away when it has 2x of its shelf life in rot.

        if( calendar::turn <= calendar::start_of_cataclysm ) {
            calendar::turn = calendar::start_of_cataclysm + 1_minutes;
        }

        item test_item( "meat_cooked" );

        // Process item once to set all of its values.
        test_item.process( nullptr, tripoint_zero, false, 1, temperature_flag::TEMP_HEATER );

        // Set rot to >2 days and process again. process_rot should return true.
        calendar::turn += 20_minutes;
        test_item.mod_rot( 2_days );

        CHECK( test_item.process_rot( 1, false, tripoint_zero, nullptr,
                                      temperature_flag::TEMP_HEATER ) );
        INFO( "Rot: " << to_turns<int>( test_item.get_rot() ) );
    }

    SECTION( "Item on map rots away" ) {
        clear_map();
        const tripoint loc;

        if( calendar::turn <= calendar::start_of_cataclysm ) {
            calendar::turn = calendar::start_of_cataclysm + 1_minutes;
        }

        item test_item( "meat_cooked" );
        test_item.process( nullptr, tripoint_zero, false, 1, temperature_flag::TEMP_HEATER );
        map &m = get_map();
        m.add_item_or_charges( loc, test_item, false );

        REQUIRE( m.i_at( loc ).size() == 1 );

        calendar::turn += 20_minutes;
        m.i_at( loc ).only_item().mod_rot( 7_days );
        m.process_items();

        CHECK( m.i_at( loc ).empty() );
    }
}
