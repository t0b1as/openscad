#include "colormap.h"
#include <boost/lexical_cast.hpp>
#include <boost/assign/list_of.hpp>
#include "boosty.h"
#include "printutils.h"
#include "PlatformUtils.h"

using namespace boost::assign; // bring map_list_of() into scope

static const char *DEFAULT_COLOR_SCHEME_NAME = "Cornfield";

RenderColorScheme::RenderColorScheme() : path("")
{
	_name = DEFAULT_COLOR_SCHEME_NAME;
	_index = 1000;
	_show_in_gui = true;

	_color_scheme.insert(ColorScheme::value_type(BACKGROUND_COLOR, Color4f(0xff, 0xff, 0xe5)));
	_color_scheme.insert(ColorScheme::value_type(OPENCSG_FACE_FRONT_COLOR, Color4f(0xf9, 0xd7, 0x2c)));
	_color_scheme.insert(ColorScheme::value_type(OPENCSG_FACE_BACK_COLOR, Color4f(0x9d, 0xcb, 0x51)));
	_color_scheme.insert(ColorScheme::value_type(CGAL_FACE_FRONT_COLOR, Color4f(0xf9, 0xd7, 0x2c)));
	_color_scheme.insert(ColorScheme::value_type(CGAL_FACE_2D_COLOR, Color4f(0x00, 0xbf, 0x99)));
	_color_scheme.insert(ColorScheme::value_type(CGAL_FACE_BACK_COLOR, Color4f(0x9d, 0xcb, 0x51)));
	_color_scheme.insert(ColorScheme::value_type(CGAL_EDGE_FRONT_COLOR, Color4f(0xff, 0xec, 0x5e)));
	_color_scheme.insert(ColorScheme::value_type(CGAL_EDGE_BACK_COLOR, Color4f(0xab, 0xd8, 0x56)));
	_color_scheme.insert(ColorScheme::value_type(CGAL_EDGE_2D_COLOR, Color4f(0xff, 0x00, 0x00)));
	_color_scheme.insert(ColorScheme::value_type(CROSSHAIR_COLOR, Color4f(0x80, 0x00, 0x00)));
}

RenderColorScheme::RenderColorScheme(fs::path path) : path(path)
{
    try {
	boost::property_tree::read_json(boosty::stringy(path).c_str(), pt);
	_name = pt.get<std::string>("name");
	_index = pt.get<int>("index");
	_show_in_gui = pt.get<bool>("show-in-gui");
	
	addColor(BACKGROUND_COLOR, "background");
	addColor(OPENCSG_FACE_FRONT_COLOR, "opencsg-face-front");
	addColor(OPENCSG_FACE_BACK_COLOR, "opencsg-face-back");
	addColor(CGAL_FACE_FRONT_COLOR, "cgal-face-front");
	addColor(CGAL_FACE_2D_COLOR, "cgal-face-2d");
	addColor(CGAL_FACE_BACK_COLOR, "cgal-face-back");
	addColor(CGAL_EDGE_FRONT_COLOR, "cgal-edge-front");
	addColor(CGAL_EDGE_BACK_COLOR, "cgal-edge-back");
	addColor(CGAL_EDGE_2D_COLOR, "cgal-edge-2d");
	addColor(CROSSHAIR_COLOR, "crosshair");
    } catch (const std::exception & e) {
	PRINTB("Error reading color scheme file '%s': %s", path.c_str() % e.what());
	_name = "";
	_index = 0;
	_show_in_gui = false;
    }
}

RenderColorScheme::~RenderColorScheme()
{
}

bool RenderColorScheme::valid() const
{
    return !_name.empty();
}

const std::string & RenderColorScheme::name() const
{
    return _name;
}

int RenderColorScheme::index() const
{
    return _index;
}

bool RenderColorScheme::showInGui() const
{
    return _show_in_gui;
}

ColorScheme & RenderColorScheme::colorScheme()
{
    return _color_scheme;
}

const boost::property_tree::ptree & RenderColorScheme::propertyTree() const
{
    return pt;
}

void RenderColorScheme::addColor(RenderColor colorKey, std::string key)
{
    const boost::property_tree::ptree& colors = pt.get_child("colors");
    std::string color = colors.get<std::string>(key);
    if ((color.length() == 7) && (color.at(0) == '#')) {
	char *endptr;
	unsigned int val = strtol(color.substr(1).c_str(), &endptr, 16);
	int r = (val >> 16) & 0xff;
	int g = (val >> 8) & 0xff;
	int b = val & 0xff;
	_color_scheme.insert(ColorScheme::value_type(colorKey, Color4f(r, g, b)));
    } else {
	throw std::invalid_argument(std::string("invalid color value for key '") + key + "': '" + color + "'");
    }
}

ColorMap *ColorMap::inst(bool erase)
{
	static ColorMap *instance = new ColorMap;
	if (erase) {
		delete instance;
		instance = NULL;
	}
	return instance;
}

ColorMap::ColorMap()
{
    colorSchemeSet = enumerateColorSchemes();
}

ColorMap::~ColorMap()
{
}

const ColorScheme &ColorMap::defaultColorScheme() const
{
    return *findColorScheme(DEFAULT_COLOR_SCHEME_NAME);
}

const ColorScheme *ColorMap::findColorScheme(const std::string &name) const
{
    for (colorscheme_set_t::const_iterator it = colorSchemeSet.begin();it != colorSchemeSet.end();it++) {
	RenderColorScheme *scheme = (*it).second.get();
	if (name == scheme->name()) {
	    return &scheme->colorScheme();
	}
    }

    return NULL;
}

std::list<std::string> ColorMap::colorSchemeNames(bool guiOnly) const
{
    std::list<std::string> colorSchemeNames;
    for (colorscheme_set_t::const_iterator it = colorSchemeSet.begin();it != colorSchemeSet.end();it++) {
	RenderColorScheme *scheme = (*it).second.get();
	if (guiOnly && !scheme->showInGui()) {
	    continue;
	}
        colorSchemeNames.push_back(scheme->name());
    }
	
    return colorSchemeNames;
}

Color4f ColorMap::getColor(const ColorScheme &cs, const RenderColor rc)
{
	if (cs.count(rc)) return cs.at(rc);
	if (ColorMap::inst()->defaultColorScheme().count(rc)) return ColorMap::inst()->defaultColorScheme().at(rc);
	return Color4f(0, 0, 0, 127);
}

void ColorMap::enumerateColorSchemesInPath(colorscheme_set_t &result_set, const fs::path path)
{
    const fs::path color_schemes = path / "color-schemes" / "render";
    
    fs::directory_iterator end_iter;
    
    if (fs::exists(color_schemes) && fs::is_directory(color_schemes)) {
	for (fs::directory_iterator dir_iter(color_schemes); dir_iter != end_iter; ++dir_iter) {
	    if (!fs::is_regular_file(dir_iter->status())) {
		continue;
	    }
	    
	    const fs::path path = (*dir_iter).path();
	    if (!(path.extension().string() == ".json")) {
		continue;
	    }
	    
	    RenderColorScheme *colorScheme = new RenderColorScheme(path);
	    if (colorScheme->valid() && (findColorScheme(colorScheme->name()) == 0)) {
		result_set.insert(colorscheme_set_t::value_type(colorScheme->index(), boost::shared_ptr<RenderColorScheme>(colorScheme)));
	    } else {
		delete colorScheme;
	    }
	}
    }
}

ColorMap::colorscheme_set_t ColorMap::enumerateColorSchemes()
{
    colorscheme_set_t result_set;

    RenderColorScheme *defaultColorScheme = new RenderColorScheme();
    result_set.insert(colorscheme_set_t::value_type(defaultColorScheme->index(),
	    boost::shared_ptr<RenderColorScheme>(defaultColorScheme)));
    enumerateColorSchemesInPath(result_set, PlatformUtils::resourcesPath());
    enumerateColorSchemesInPath(result_set, PlatformUtils::userConfigPath());
    
    return result_set;
}