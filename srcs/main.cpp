#include "Zia.hpp"

int main(int ac, char **av) {
	zany::Context	ctx;
	zia::Main		main(ctx);

	main.run(ac, av);
	return 0;
}