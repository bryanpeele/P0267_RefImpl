#include "catch.hpp"
#include <io2d.h>
#include "comparison.h"

using namespace std;
using namespace std::experimental;
using namespace std::experimental::io2d;

/*
Current state of blend operators compatibility:

Opeator         | Tested? | Cairo     | CoreGraphics  |
===================================================================================================
over            |   yes   |   +       |     +         |
clear           |   yes   |   +       |     +         |
source          |   yes   |   +       |     +         |
in              |   yes   |   +       |     +         |
out             |   yes   |   +       |     +         |
atop            |   yes   |   +       |     +         |
dest            |   NO!!! |   +       |    N/A        |
dest_over       |   yes   |   +       |     +         |
dest_in         |   yes   |   +       |     +         |
dest_out        |   yes   |   +       |     +         |
dest_atop       |   yes   |   +       |     +         |
xor_op          |   yes   |   +       |     +         |
add             |   yes   |   +       |     +         |
saturate        |   NO!!! |   +       |    N/A        |
multiply        |         |   +       |     ?         |
screen          |         |   +       |     ?         |
overlay         |         |   +       |     ?         |
darken          |         |   +       |     ?         |
lighten         |         |   +       |     ?         |
color_dodge     |         |   +       |     ?         |
color_burn      |         |   +       |     ?         |
hard_light      |         |   +       |     ?         |
soft_light      |         |   +       |     ?         |
difference      |         |   +       |     ?         |
exclusion       |         |   +       |     ?         |
hsl_hue         |         |   +       |     ?         |
hsl_saturation  |         |   +       |     ?         |
hsl_color       |         |   +       |     ?         |
hsl_luminosity  |         |   +       |     ?         |
===================================================================================================
*/


namespace Blend {
// zero alpha processing does not match the mathetical model in some operators,
// which is coherent between Cairo and CoreGraphics.
    
static rgba_color Clear( const rgba_color& a, const rgba_color& b ) noexcept
{
    return rgba_color::transparent_black;
}

static rgba_color Over( const rgba_color& a, const rgba_color& b ) noexcept
{
    auto inv = 1.f - a.a();
    auto ra = a.a() + b.a() * inv;
    if( ra <= numeric_limits<float>::min() )
        return rgba_color::transparent_black;
    auto rr = (a.r() * a.a() + b.r() * b.a() * inv) / ra;
    auto rg = (a.g() * a.a() + b.g() * b.a() * inv) / ra;
    auto rb = (a.b() * a.a() + b.b() * b.a() * inv) / ra;
    return {rr, rg, rb, ra};
}
    
static rgba_color Source( const rgba_color& a, const rgba_color& b ) noexcept
{
    if( a.a() <= numeric_limits<float>::min() )
        return rgba_color::transparent_black;
    return a;
}
    
static rgba_color In( const rgba_color& a, const rgba_color& b ) noexcept
{
    auto ra = a.a() * b.a();
    if( ra <= numeric_limits<float>::min() )
        return rgba_color::transparent_black;
    return {a.r(), a.g(), a.b(), ra};
}

static rgba_color Out( const rgba_color& a, const rgba_color& b ) noexcept
{
    auto ra = a.a() * (1.f - b.a());
    if( ra <= numeric_limits<float>::min() )
        return rgba_color::transparent_black;
    return {a.r(), a.g(), a.b(), ra};
}

static rgba_color Atop( const rgba_color& a, const rgba_color& b ) noexcept
{
    auto ra = b.a();
    if( ra <= numeric_limits<float>::min() )
        return rgba_color::transparent_black;
    auto inv = 1.f - a.a();
    auto rr = a.r() * a.a() + b.r() * inv;
    auto rg = a.g() * a.a() + b.g() * inv;
    auto rb = a.b() * a.a() + b.b() * inv;
    return {rr, rg, rb, ra};
}

static rgba_color DestOver( const rgba_color& a, const rgba_color& b ) noexcept
{
    auto inv = 1.f - b.a();
    auto ra = a.a() * inv + b.a();
    if( ra <= numeric_limits<float>::min() )
        return rgba_color::transparent_black;
    auto rr = (a.r() * a.a() * inv + b.r() * b.a() ) / ra;
    auto rg = (a.g() * a.a() * inv + b.g() * b.a() ) / ra;
    auto rb = (a.b() * a.a() * inv + b.b() * b.a() ) / ra;
    return {rr, rg, rb, ra};
}
    
static rgba_color DestIn( const rgba_color& a, const rgba_color& b ) noexcept
{
    auto ra = a.a() * b.a();
    if( ra <= numeric_limits<float>::min() )
        return rgba_color::transparent_black;
    return {b.r(), b.g(), b.b(), ra};
}
    
static rgba_color DestOut( const rgba_color& a, const rgba_color& b ) noexcept
{
    auto ra = (1.f - a.a()) * b.a();
    if( ra <= numeric_limits<float>::min() )
        return rgba_color::transparent_black;
    return {b.r(), b.g(), b.b(), ra};
}

static rgba_color DestAtop( const rgba_color& a, const rgba_color& b ) noexcept
{
    auto ra = a.a();
    if( ra <= numeric_limits<float>::min() )
        return rgba_color::transparent_black;
    auto inv = 1.f - b.a();
    auto rr = a.r() * inv + b.r() * b.a();
    auto rg = a.g() * inv + b.g() * b.a();
    auto rb = a.b() * inv + b.b() * b.a();
    return {rr, rg, rb, ra};
}

static rgba_color Xor( const rgba_color& a, const rgba_color& b ) noexcept
{
    auto ra = a.a() + b.a() - 2.f * a.a() * b.a();
    if( ra <= numeric_limits<float>::min() )
        return rgba_color::transparent_black;
    auto rr = (a.r() * a.a() * (1.f - b.a()) + b.r() * b.a() * (1.f - a.a()) ) / ra;
    auto rg = (a.g() * a.a() * (1.f - b.a()) + b.g() * b.a() * (1.f - a.a()) ) / ra;
    auto rb = (a.b() * a.a() * (1.f - b.a()) + b.b() * b.a() * (1.f - a.a()) ) / ra;
    return {rr, rg, rb, ra};
}
    
static rgba_color Add( const rgba_color& a, const rgba_color& b ) noexcept
{
    auto ra = min( 1.f, a.a() + b.a() );
    if( ra <= numeric_limits<float>::min() )
        return rgba_color::transparent_black;
    auto rr = ( a.r() * a.a() + b.r() * b.a() ) / ra;
    auto rg = ( a.g() * a.a() + b.g() * b.a() ) / ra;
    auto rb = ( a.b() * a.a() + b.b() * b.a() ) / ra;
    return {rr, rg, rb, ra};
}
    
}

static function<rgba_color(const rgba_color&, const rgba_color&)> BlendFunction( compositing_op op  )
{
    switch (op) {
        case compositing_op::over:      return Blend::Over;
        case compositing_op::clear:     return Blend::Clear;
        case compositing_op::source:    return Blend::Source;
        case compositing_op::in:        return Blend::In;
        case compositing_op::out:       return Blend::Out;
        case compositing_op::atop:      return Blend::Atop;
        case compositing_op::dest_over: return Blend::DestOver;
        case compositing_op::dest_in:   return Blend::DestIn;
        case compositing_op::dest_out:  return Blend::DestOut;
        case compositing_op::dest_atop: return Blend::DestAtop;
        case compositing_op::xor_op:    return Blend::Xor;
        case compositing_op::add:       return Blend::Add;
        default: return nullptr;
    }
}

using Triplet = tuple<rgba_color, rgba_color, rgba_color>;

// returns 625 combinations of "A blend B = C", iterating over one color component and over alpha component
static vector<Triplet> BuildRefData( function<rgba_color(const rgba_color&, const rgba_color&)> blend )
{
    assert( blend );
    auto step = 51;
    vector<Triplet> colors;
    for( int a_r = 0; a_r <= 255; a_r += step )
        for( int a_a = 0; a_a <= 255; a_a += step )
            for( int b_r = 0; b_r <= 255; b_r += step )
                for( int b_a = 0; b_a <= 255; b_a += step ) {
                    auto a = rgba_color{ a_r, 0, 0, a_a };
                    auto b = rgba_color{ b_r, 0, 0, b_a };
                    auto c = blend(a, b);
                    colors.emplace_back(a, b, c);
                }
    return colors;
}

// TODO: move it into IO2D itself?
namespace std::experimental::io2d {
static std::ostream& operator<<(std::ostream& stream, const rgba_color& color) {
    stream << "{";
    stream << to_string(static_cast<int>(color.r() * 255.f)) << ",";
    stream << to_string(static_cast<int>(color.g() * 255.f)) << ",";
    stream << to_string(static_cast<int>(color.b() * 255.f)) << ",";
    stream << to_string(static_cast<int>(color.a() * 255.f)) << "}";
    return stream;
}
}

TEST_CASE("IO2D properly blends colors using compositing_op::over")
{
    auto op = compositing_op::over;
    auto rp = render_props{};
    rp.compositing(op);
    auto colors = BuildRefData( BlendFunction(rp.compositing()) );
    for( auto &t: colors ) {
        auto img = image_surface{format::argb32, 1, 1};
        img.paint(brush{get<1>(t)});
        img.paint(brush{get<0>(t)}, nullopt, rp);
        INFO("A: " << get<0>(t) << ", B: " << get<1>(t) << ", C: " << get<2>(t) );
        CHECK( CheckPNGColorWithTolerance(img, 0, 0, get<2>(t), 0.05) == true);
    }
}

TEST_CASE("IO2D properly blends colors using compositing_op::clear")
{
    auto op = compositing_op::clear;
    auto rp = render_props{};
    rp.compositing(op);
    auto colors = BuildRefData( BlendFunction(rp.compositing()) );
    for( auto &t: colors ) {
        auto img = image_surface{format::argb32, 1, 1};
        img.paint(brush{get<1>(t)});
        img.paint(brush{get<0>(t)}, nullopt, rp);
        INFO("A: " << get<0>(t) << ", B: " << get<1>(t) << ", C: " << get<2>(t) );
        CHECK( CheckPNGColorWithTolerance(img, 0, 0, get<2>(t), 0.05) == true);
    }
}

TEST_CASE("IO2D properly blends colors using compositing_op::source")
{
    auto op = compositing_op::source;
    auto rp = render_props{};
    rp.compositing(op);
    auto colors = BuildRefData( BlendFunction(rp.compositing()) );
    for( auto &t: colors ) {
        auto img = image_surface{format::argb32, 1, 1};
        img.paint(brush{get<1>(t)});
        img.paint(brush{get<0>(t)}, nullopt, rp);
        INFO("A: " << get<0>(t) << ", B: " << get<1>(t) << ", C: " << get<2>(t) );
        CHECK( CheckPNGColorWithTolerance(img, 0, 0, get<2>(t), 0.05) == true);
    }
}

TEST_CASE("IO2D properly blends colors using compositing_op::in")
{
    auto op = compositing_op::in;
    auto rp = render_props{};
    rp.compositing(op);
    auto colors = BuildRefData( BlendFunction(rp.compositing()) );
    for( auto &t: colors ) {
        auto img = image_surface{format::argb32, 1, 1};
        img.paint(brush{get<1>(t)});
        img.paint(brush{get<0>(t)}, nullopt, rp);
        INFO("A: " << get<0>(t) << ", B: " << get<1>(t) << ", C: " << get<2>(t) );
        CHECK( CheckPNGColorWithTolerance(img, 0, 0, get<2>(t), 0.05) == true);
    }
}

TEST_CASE("IO2D properly blends colors using compositing_op::out")
{
    auto op = compositing_op::out;
    auto rp = render_props{};
    rp.compositing(op);
    auto colors = BuildRefData( BlendFunction(rp.compositing()) );
    for( auto &t: colors ) {
        auto img = image_surface{format::argb32, 1, 1};
        img.paint(brush{get<1>(t)});
        img.paint(brush{get<0>(t)}, nullopt, rp);
        INFO("A: " << get<0>(t) << ", B: " << get<1>(t) << ", C: " << get<2>(t) );
        CHECK( CheckPNGColorWithTolerance(img, 0, 0, get<2>(t), 0.05) == true);
    }
}

TEST_CASE("IO2D properly blends colors using compositing_op::atop")
{
    auto op = compositing_op::atop;
    auto rp = render_props{};
    rp.compositing(op);
    auto colors = BuildRefData( BlendFunction(rp.compositing()) );
    for( auto &t: colors ) {
        auto img = image_surface{format::argb32, 1, 1};
        img.paint(brush{get<1>(t)});
        img.paint(brush{get<0>(t)}, nullopt, rp);
        INFO("A: " << get<0>(t) << ", B: " << get<1>(t) << ", C: " << get<2>(t) );
        CHECK( CheckPNGColorWithTolerance(img, 0, 0, get<2>(t), 0.05) == true);
    }
}

TEST_CASE("IO2D properly blends colors using compositing_op::dest_over")
{
    auto op = compositing_op::dest_over;
    auto rp = render_props{};
    rp.compositing(op);
    auto colors = BuildRefData( BlendFunction(rp.compositing()) );
    for( auto &t: colors ) {
        auto img = image_surface{format::argb32, 1, 1};
        img.paint(brush{get<1>(t)});
        img.paint(brush{get<0>(t)}, nullopt, rp);
        INFO("A: " << get<0>(t) << ", B: " << get<1>(t) << ", C: " << get<2>(t) );
        CHECK( CheckPNGColorWithTolerance(img, 0, 0, get<2>(t), 0.05) == true);
    }
}

TEST_CASE("IO2D properly blends colors using compositing_op::dest_in")
{
    auto op = compositing_op::dest_in;
    auto rp = render_props{};
    rp.compositing(op);
    auto colors = BuildRefData( BlendFunction(rp.compositing()) );
    for( auto &t: colors ) {
        auto img = image_surface{format::argb32, 1, 1};
        img.paint(brush{get<1>(t)});
        img.paint(brush{get<0>(t)}, nullopt, rp);
        INFO("A: " << get<0>(t) << ", B: " << get<1>(t) << ", C: " << get<2>(t) );
        CHECK( CheckPNGColorWithTolerance(img, 0, 0, get<2>(t), 0.05) == true);
    }
}

TEST_CASE("IO2D properly blends colors using compositing_op::dest_out")
{
    auto op = compositing_op::dest_out;
    auto rp = render_props{};
    rp.compositing(op);
    auto colors = BuildRefData( BlendFunction(rp.compositing()) );
    for( auto &t: colors ) {
        auto img = image_surface{format::argb32, 1, 1};
        img.paint(brush{get<1>(t)});
        img.paint(brush{get<0>(t)}, nullopt, rp);
        INFO("A: " << get<0>(t) << ", B: " << get<1>(t) << ", C: " << get<2>(t) );
        CHECK( CheckPNGColorWithTolerance(img, 0, 0, get<2>(t), 0.05) == true);
    }
}

TEST_CASE("IO2D properly blends colors using compositing_op::dest_atop")
{
    auto op = compositing_op::dest_atop;
    auto rp = render_props{};
    rp.compositing(op);
    auto colors = BuildRefData( BlendFunction(rp.compositing()) );
    for( auto &t: colors ) {
        auto img = image_surface{format::argb32, 1, 1};
        img.paint(brush{get<1>(t)});
        img.paint(brush{get<0>(t)}, nullopt, rp);
        INFO("A: " << get<0>(t) << ", B: " << get<1>(t) << ", C: " << get<2>(t) );
        CHECK( CheckPNGColorWithTolerance(img, 0, 0, get<2>(t), 0.05) == true);
    }
}

TEST_CASE("IO2D properly blends colors using compositing_op::xor_op")
{
    auto op = compositing_op::xor_op;
    auto rp = render_props{};
    rp.compositing(op);
    auto colors = BuildRefData( BlendFunction(rp.compositing()) );
    for( auto &t: colors ) {
        auto img = image_surface{format::argb32, 1, 1};
        img.paint(brush{get<1>(t)});
        img.paint(brush{get<0>(t)}, nullopt, rp);
        INFO("A: " << get<0>(t) << ", B: " << get<1>(t) << ", C: " << get<2>(t) );
        CHECK( CheckPNGColorWithTolerance(img, 0, 0, get<2>(t), 0.05) == true);
    }
}

TEST_CASE("IO2D properly blends colors using compositing_op::add")
{
    auto op = compositing_op::add;
    auto rp = render_props{};
    rp.compositing(op);
    auto colors = BuildRefData( BlendFunction(rp.compositing()) );
    for( auto &t: colors ) {
        auto img = image_surface{format::argb32, 1, 1};
        img.paint(brush{get<1>(t)});
        img.paint(brush{get<0>(t)}, nullopt, rp);
        INFO("A: " << get<0>(t) << ", B: " << get<1>(t) << ", C: " << get<2>(t) );
        CHECK( CheckPNGColorWithTolerance(img, 0, 0, get<2>(t), 0.05) == true);
    }
}
