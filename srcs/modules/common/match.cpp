#include <string>
#include <algorithm>
#include <iostream>
#include <vector>
#include "Zany/Entity.hpp"

template<typename ForwardIt>
ForwardIt getBestMatch(ForwardIt beg, ForwardIt end, std::string const &str)
{
	auto b = [](auto s1, auto s2) -> bool {
		if (*s2 == '*')
			return (*s1 ? match(s1 + 1, s2) || match(s1, s2 + 1) : match(s1, s2 + 1));
		if (*s1 == *s2)
				return (*s1 ? match(s1 + 1, s2 + 1) : true);
		return false;
	};

	zany::Entity e1, e2;

	e1.merge(e2, [] (auto &first, auto &second) {
		
	});

	std::vector<ForwardIt> matches;

	for (;beg != end; ++beg) {
		if (b(str.data(), beg->data()))
			matches.emplace_back(beg);
	}
	if (matches.size() == 0)
		return end;
	return *std::max_element(matches.begin(), matches.end(), [](auto &&s1, auto &&s2) {
		return s1->length() < s2->length();
	});
}

int main(int ac, char **av)
{
	if (ac < 3)
		return 1;
	std::string str{av[1]};
	std::vector<std::string> vec;

	for (auto i = 2; i < ac; ++i) {
		vec.emplace_back(av[i]);
	}

	auto &&a = getBestMatch(vec.begin(), vec.end(), str);

	if (a != vec.end()) {
		std::cout << "best : " << *a << std::endl;
	} else {
		std::cout << "not found" << std::endl;
	}
}