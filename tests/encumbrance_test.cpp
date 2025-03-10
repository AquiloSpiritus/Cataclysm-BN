#include "catch/catch.hpp"

#include <array>
#include <functional>
#include <list>
#include <memory>
#include <string>
#include <vector>

#include "avatar.h"
#include "bodypart.h"
#include "character.h"
#include "character_encumbrance.h"
#include "game.h"
#include "item.h"
#include "material.h"
#include "npc.h"
#include "player.h"
#include "type_id.h"

static void test_encumbrance_on(
    player &p,
    const std::vector<item> &clothing,
    const std::string &body_part,
    int expected_encumbrance,
    const std::function<void( player & )> &tweak_player = {}
)
{
    CAPTURE( body_part );
    p.set_body();
    p.clear_mutations();
    p.worn.clear();
    if( tweak_player ) {
        tweak_player( p );
    }
    for( const item &i : clothing ) {
        p.worn.push_back( i );
    }
    p.reset_encumbrance();
    encumbrance_data enc = p.get_encumbrance().elems[ get_body_part_token( body_part ) ];
    CHECK( enc.encumbrance == expected_encumbrance );
}

static void test_encumbrance_items(
    const std::vector<item> &clothing,
    const std::string &body_part,
    const int expected_encumbrance,
    const std::function<void( player & )> &tweak_player = {}
)
{
    // Test NPC first because NPC code can accidentally end up using properties
    // of g->u, and such bugs are hidden if we test the other way around.
    SECTION( "testing on npc" ) {
        npc example_npc;
        test_encumbrance_on( example_npc, clothing, body_part, expected_encumbrance, tweak_player );
    }
    SECTION( "testing on player" ) {
        test_encumbrance_on( get_avatar(), clothing, body_part, expected_encumbrance, tweak_player );
    }
}

static void test_encumbrance(
    const std::vector<std::string> &clothing_types,
    const std::string &body_part,
    const int expected_encumbrance
)
{
    CAPTURE( clothing_types );
    std::vector<item> clothing;
    for( const std::string &type : clothing_types ) {
        clothing.push_back( item( type ) );
    }
    test_encumbrance_items( clothing, body_part, expected_encumbrance );
}

struct add_trait {
    add_trait( const std::string &t ) : trait( t ) {}
    add_trait( const trait_id &t ) : trait( t ) {}

    void operator()( player &p ) {
        p.toggle_trait( trait );
    }

    trait_id trait;
};

static constexpr int postman_shirt_e = 0;
static constexpr int longshirt_e = 3;
static constexpr int jacket_jean_e = 11;

TEST_CASE( "regular_clothing_encumbrance", "[encumbrance]" )
{
    test_encumbrance( { "postman_shirt" }, "TORSO", postman_shirt_e );
    test_encumbrance( { "longshirt" }, "TORSO", longshirt_e );
    test_encumbrance( { "jacket_jean" }, "TORSO", jacket_jean_e );
}

TEST_CASE( "separate_layer_encumbrance", "[encumbrance]" )
{
    test_encumbrance( { "longshirt", "jacket_jean" }, "TORSO", longshirt_e + jacket_jean_e );
}

TEST_CASE( "out_of_order_encumbrance", "[encumbrance]" )
{
    test_encumbrance( { "jacket_jean", "longshirt" }, "TORSO", longshirt_e * 2 + jacket_jean_e );
}

TEST_CASE( "same_layer_encumbrance", "[encumbrance]" )
{
    // When stacking within a layer, encumbrance for additional items is
    // counted twice
    test_encumbrance( { "longshirt", "longshirt" }, "TORSO", longshirt_e * 2 + longshirt_e );
    // ... with a minimum of 2
    test_encumbrance( { "postman_shirt", "postman_shirt" }, "TORSO", postman_shirt_e * 2 + 2 );
    // ... and a maximum of 10
    test_encumbrance( { "jacket_jean", "jacket_jean" }, "TORSO", jacket_jean_e * 2 + 10 );
}

TEST_CASE( "tiny_clothing", "[encumbrance]" )
{
    item i( "longshirt" );
    i.set_flag( "UNDERSIZE" );
    test_encumbrance_items( { i }, "TORSO", longshirt_e * 3 );
}

TEST_CASE( "tiny_character", "[encumbrance]" )
{
    item i( "longshirt" );
    SECTION( "regular shirt" ) {
        test_encumbrance_items( { i }, "TORSO", longshirt_e * 2, add_trait( "SMALL2" ) );
    }
    SECTION( "undersize shrt" ) {
        i.set_flag( "UNDERSIZE" );
        test_encumbrance_items( { i }, "TORSO", longshirt_e, add_trait( "SMALL2" ) );
    }
}
