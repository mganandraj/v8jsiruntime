#include <typedjsi.h>
#include <jsi/jsi.h>

namespace typedjsi {

template <>
double get<double>(facebook::jsi::Runtime&, const facebook::jsi::Value& value) noexcept
{
	assert(value.isNumber());

	return value.getNumber();
}

template <>
std::int32_t get<std::int32_t>(facebook::jsi::Runtime&, const facebook::jsi::Value& value) noexcept
{
	assert(value.isNumber());

	return static_cast<std::int32_t>(value.getNumber());
}

template <>
std::string get<std::string>(facebook::jsi::Runtime& runtime, const facebook::jsi::Value& value) noexcept
{
	assert(value.isString());

	return value.getString(runtime).utf8(runtime);
}

template <>
bool get<bool>(facebook::jsi::Runtime&, const facebook::jsi::Value& value) noexcept
{
	assert(value.isBool());

	return value.getBool();
}

} // namespace typedjsi
