#include <string>
#include <algorithm>
#include <iostream>
#include <vector>
#include <boost/optional.hpp>
#include "Zany/Entity.hpp"

bool match(char const *s1, char const *s2)
{
	if (*s2 == '*')
		return (*s1 ? match(s1 + 1, s2) || match(s1, s2 + 1) : match(s1, s2 + 1));
	if (*s1 == *s2)
			return (*s1 ? match(s1 + 1, s2 + 1) : true);
	return false;
}

template<typename ForwardIt>
ForwardIt getBestMatch(ForwardIt beg, ForwardIt end, std::string const &str)
{
	std::vector<ForwardIt> matches;

	for (;beg != end; ++beg) {
		if (match(str.data(), beg->data()))
			matches.emplace_back(beg);
	}
	if (matches.size() == 0)
		return end;
	return *std::max_element(matches.begin(), matches.end(), [](auto &&s1, auto &&s2) {
		return s1->length() < s2->length();
	});
}

using ForwardIT = std::vector<zany::Entity>::iterator;

boost::optional<zany::Entity> getBestEntity(ForwardIT beg, ForwardIT end, std::string const &str)
{
	std::vector<ForwardIT> matches;

	for (;beg != end; ++beg) {
		if (match(str.data(), (*beg)["host"].value<zany::String>().data()))
			matches.emplace_back(beg);
	}
	if (matches.size() == 0)
		return boost::optional<zany::Entity>{};

	zany::Entity mergedEntity = zany::makeObject{};

	std::stable_sort(matches.begin(), matches.end(), [] (ForwardIT e1, ForwardIT e2) {
		return (*e1)["host"].value<zany::String>().length() < (*e2)["host"].value<zany::String>().length();
	});
	for (auto const &e : matches) {
		mergedEntity.merge(*e, [] (std::string const &key, auto &first, auto &second) {
			first = second;
		});
	}
	return mergedEntity;
}

int main(int ac, char **av)
{
	if (ac != 2)
		return 1;
	std::string str{av[1]};
	std::vector<zany::Entity> vec = {
		zany::makeObject{{"host", "www.*.com"}, {"oui", "non"}, {"php", "ok"}},
		zany::makeObject{{"host", "www.zia.*.com"}, {"oui", "oui"}},
	};

	auto &&a = getBestEntity(vec.begin(), vec.end(), str);

	if (a) {
		std::cout << "best : " << std::endl;
		for (auto &e : a->value<zany::Object>()) {
			std::cout << "best : " << e.first << "  " << e.second.value<zany::String>() << std::endl;
		}
	} else {
		std::cout << "not found" << std::endl;
	}
}