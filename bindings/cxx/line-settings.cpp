// SPDX-License-Identifier: LGPL-3.0-or-later
// SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <brgl@bgdev.pl>

#include <map>
#include <ostream>

#include "internal.hpp"

namespace gpiod {

namespace {

template<class cxx_enum_type, class c_enum_type>
::std::map<c_enum_type, cxx_enum_type>
make_reverse_maping(const ::std::map<cxx_enum_type, c_enum_type>& mapping)
{
	::std::map<c_enum_type, cxx_enum_type> ret;

	for (const auto &item: mapping)
		ret[item.second] = item.first;

	return ret;
}

const ::std::map<line::direction, gpiod_line_direction> direction_mapping = {
	{ line::direction::AS_IS,	GPIOD_LINE_DIRECTION_AS_IS },
	{ line::direction::INPUT,	GPIOD_LINE_DIRECTION_INPUT },
	{ line::direction::OUTPUT,	GPIOD_LINE_DIRECTION_OUTPUT },
};

const ::std::map<gpiod_line_direction, line::direction>
reverse_direction_mapping = make_reverse_maping(direction_mapping);

const ::std::map<line::edge, gpiod_line_edge> edge_mapping = {
	{ line::edge::NONE,		GPIOD_LINE_EDGE_NONE },
	{ line::edge::FALLING,		GPIOD_LINE_EDGE_FALLING },
	{ line::edge::RISING,		GPIOD_LINE_EDGE_RISING },
	{ line::edge::BOTH,		GPIOD_LINE_EDGE_BOTH },
};

const ::std::map<gpiod_line_edge, line::edge> reverse_edge_mapping = make_reverse_maping(edge_mapping);

const ::std::map<line::bias, gpiod_line_bias> bias_mapping = {
	{ line::bias::AS_IS,		GPIOD_LINE_BIAS_AS_IS },
	{ line::bias::DISABLED,		GPIOD_LINE_BIAS_DISABLED },
	{ line::bias::PULL_UP,		GPIOD_LINE_BIAS_PULL_UP },
	{ line::bias::PULL_DOWN,	GPIOD_LINE_BIAS_PULL_DOWN },
};

const ::std::map<gpiod_line_bias, line::bias> reverse_bias_mapping = make_reverse_maping(bias_mapping);

const ::std::map<line::drive, gpiod_line_drive> drive_mapping = {
	{ line::drive::PUSH_PULL,	GPIOD_LINE_DRIVE_PUSH_PULL },
	{ line::drive::OPEN_DRAIN,	GPIOD_LINE_DRIVE_OPEN_DRAIN },
	{ line::drive::OPEN_SOURCE,	GPIOD_LINE_DRIVE_OPEN_SOURCE },
};

const ::std::map<gpiod_line_drive, line::drive> reverse_drive_mapping = make_reverse_maping(drive_mapping);

const ::std::map<line::clock, gpiod_line_clock> clock_mapping = {
	{ line::clock::MONOTONIC,	GPIOD_LINE_CLOCK_MONOTONIC },
	{ line::clock::REALTIME,	GPIOD_LINE_CLOCK_REALTIME },
	{ line::clock::HTE,		GPIOD_LINE_CLOCK_HTE },
};

const ::std::map<gpiod_line_clock, line::clock>
reverse_clock_mapping = make_reverse_maping(clock_mapping);

const ::std::map<line::value, gpiod_line_value> value_mapping = {
	{ line::value::INACTIVE,	GPIOD_LINE_VALUE_INACTIVE },
	{ line::value::ACTIVE,		GPIOD_LINE_VALUE_ACTIVE },
};

const ::std::map<gpiod_line_value, line::value> reverse_value_mapping = make_reverse_maping(value_mapping);

line_settings_ptr make_line_settings()
{
	line_settings_ptr settings(::gpiod_line_settings_new());
	if (!settings)
		throw_from_errno("Unable to allocate the line settings object");

	return settings;
}

template<class key_type, class value_type, class exception_type>
value_type map_setting(const key_type& key, const ::std::map<key_type, value_type>& mapping)
{
	value_type ret;

	try {
		ret = mapping.at(key);
	} catch (const ::std::out_of_range& err) {
		/* FIXME Demangle the name. */
		throw exception_type(::std::string("invalid value for ") +
				     typeid(key_type).name());
	}

	return ret;
}

template<class cxx_enum_type, class c_enum_type>
c_enum_type do_map_value(cxx_enum_type value, const ::std::map<cxx_enum_type, c_enum_type>& mapping)
{
	return map_setting<cxx_enum_type, c_enum_type, ::std::invalid_argument>(value, mapping);
}

template<class cxx_enum_type, class c_enum_type, int set_func(::gpiod_line_settings*, c_enum_type)>
void set_mapped_value(::gpiod_line_settings* settings, cxx_enum_type value,
		      const ::std::map<cxx_enum_type, c_enum_type>& mapping)
{
	c_enum_type mapped_val = do_map_value(value, mapping);

	auto ret = set_func(settings, mapped_val);
	if (ret)
		throw_from_errno("unable to set property");
}

template<class cxx_enum_type, class c_enum_type, c_enum_type get_func(::gpiod_line_settings*)>
cxx_enum_type get_mapped_value(::gpiod_line_settings* settings,
			  const ::std::map<c_enum_type, cxx_enum_type>& mapping)
{
	auto mapped_val = get_func(settings);

	return map_enum_c_to_cxx(mapped_val, mapping);
}

} /* namespace */

line_settings::impl::impl()
	: settings(make_line_settings())
{

}

GPIOD_CXX_API line_settings::line_settings()
	: _m_priv(new impl)
{

}

GPIOD_CXX_API line_settings::line_settings(line_settings&& other) noexcept
	: _m_priv(::std::move(other._m_priv))
{

}

GPIOD_CXX_API line_settings::~line_settings()
{

}

GPIOD_CXX_API line_settings& line_settings::operator=(line_settings&& other)
{
	this->_m_priv = ::std::move(other._m_priv);

	return *this;
}

GPIOD_CXX_API line_settings& line_settings::reset(void) noexcept
{
	::gpiod_line_settings_reset(this->_m_priv->settings.get());

	return *this;
}

GPIOD_CXX_API line_settings& line_settings::set_direction(line::direction direction)
{
	set_mapped_value<line::direction, gpiod_line_direction,
			 ::gpiod_line_settings_set_direction>(this->_m_priv->settings.get(),
							      direction, direction_mapping);

	return *this;
}

GPIOD_CXX_API line::direction line_settings::direction() const
{
	return get_mapped_value<line::direction, gpiod_line_direction,
				::gpiod_line_settings_get_direction>(
							this->_m_priv->settings.get(),
							reverse_direction_mapping);
}

GPIOD_CXX_API line_settings& line_settings::set_edge_detection(line::edge edge)
{
	set_mapped_value<line::edge, gpiod_line_edge,
			 ::gpiod_line_settings_set_edge_detection>(this->_m_priv->settings.get(),
								   edge, edge_mapping);

	return *this;
}

GPIOD_CXX_API line::edge line_settings::edge_detection() const
{
	return get_mapped_value<line::edge, gpiod_line_edge,
				::gpiod_line_settings_get_edge_detection>(
							this->_m_priv->settings.get(),
							reverse_edge_mapping);
}

GPIOD_CXX_API line_settings& line_settings::set_bias(line::bias bias)
{
	set_mapped_value<line::bias, gpiod_line_bias,
			 ::gpiod_line_settings_set_bias>(this->_m_priv->settings.get(),
							 bias, bias_mapping);

	return *this;
}

GPIOD_CXX_API line::bias line_settings::bias() const
{
	return get_mapped_value<line::bias, gpiod_line_bias,
				::gpiod_line_settings_get_bias>(this->_m_priv->settings.get(),
								reverse_bias_mapping);
}

GPIOD_CXX_API line_settings& line_settings::set_drive(line::drive drive)
{
	set_mapped_value<line::drive, gpiod_line_drive,
			 ::gpiod_line_settings_set_drive>(this->_m_priv->settings.get(),
							  drive, drive_mapping);

	return *this;
}

GPIOD_CXX_API line::drive line_settings::drive() const
{
	return get_mapped_value<line::drive, gpiod_line_drive,
				::gpiod_line_settings_get_drive>(this->_m_priv->settings.get(),
								 reverse_drive_mapping);
}

GPIOD_CXX_API line_settings& line_settings::set_active_low(bool active_low)
{
	::gpiod_line_settings_set_active_low(this->_m_priv->settings.get(), active_low);

	return *this;
}

GPIOD_CXX_API bool line_settings::active_low() const noexcept
{
	return ::gpiod_line_settings_get_active_low(this->_m_priv->settings.get());
}

GPIOD_CXX_API line_settings&
line_settings::set_debounce_period(const ::std::chrono::microseconds& period)
{
	::gpiod_line_settings_set_debounce_period_us(this->_m_priv->settings.get(), period.count());

	return *this;
}

GPIOD_CXX_API ::std::chrono::microseconds line_settings::debounce_period() const noexcept
{
	return ::std::chrono::microseconds(
			::gpiod_line_settings_get_debounce_period_us(this->_m_priv->settings.get()));
}

GPIOD_CXX_API line_settings& line_settings::set_event_clock(line::clock event_clock)
{
	set_mapped_value<line::clock, gpiod_line_clock,
			 ::gpiod_line_settings_set_event_clock>(this->_m_priv->settings.get(),
								event_clock, clock_mapping);

	return *this;
}

GPIOD_CXX_API line::clock line_settings::event_clock() const
{
	return get_mapped_value<line::clock, gpiod_line_clock,
				::gpiod_line_settings_get_event_clock>(
							this->_m_priv->settings.get(),
							reverse_clock_mapping);
}

GPIOD_CXX_API line_settings& line_settings::set_output_value(line::value value)
{
	set_mapped_value<line::value, gpiod_line_value,
			 ::gpiod_line_settings_set_output_value>(this->_m_priv->settings.get(),
								 value, value_mapping);

	return *this;
}

GPIOD_CXX_API line::value line_settings::output_value() const
{
	return get_mapped_value<line::value, gpiod_line_value,
				::gpiod_line_settings_get_output_value>(
							this->_m_priv->settings.get(),
							reverse_value_mapping);
}

GPIOD_CXX_API ::std::ostream& operator<<(::std::ostream& out, const line_settings& settings)
{
	out << "gpiod::line_settings(direction=" << settings.direction() <<
	       ", edge_detection=" << settings.edge_detection() <<
	       ", bias=" << settings.bias() <<
	       ", drive=" << settings.drive() <<
	       ", " << (settings.active_low() ? "active-low" : "active-high") <<
	       ", debounce_period=" << settings.debounce_period().count() <<
	       ", event_clock=" << settings.event_clock() <<
	       ", output_value=" << settings.output_value() <<
	       ")";

	return out;
}

} /* namespace gpiod */
