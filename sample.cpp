#include <stdio.h>

struct request {
	char reply[1024] = "";
	char uri[256] = "";
	int result = -1;
};

bool wget(request& req) {
	char cmd[256];
	snprintf(cmd, 256, "wget -qO - %s", req.uri);

	req.result = -1;
	FILE* f = popen(cmd, "r");
	if (f) {
		req.reply[fread(req.reply, sizeof(req.reply) - 1, 1, f)] = '\0';
		req.result = pclose(f);
	}
	return req.result == 0;
}

// sys {{{
template<int = 0> class init {
	static inline init instance{};
	init() noexcept {
		snprintf(list[0].uri, 256, "example.comm");
		snprintf(list[1].uri, 256, "examplee.com");
		n = 2;
	}
public:
	static inline size_t n;
	static inline request list[2]{};
};

template class init<>;
using sys = init<>;
// }}}

#include "tdd.h"

// custom printer
namespace test {
	const printer_t& operator<<(const printer_t& p, const request& req) { return p.print("[%d] %s\n", req.result, req.uri); }
}

TEST(test_wget) {
	EXPECT(sys::n > 0);
	for (size_t i = 0; i < sys::n; ++i)
		EXPECT(wget(sys::list[i])) << sys::list[i];
}

// Tests within the same category and translation unit are executed in the order they were declared.
static inline int order = 0;
TEST(test_order0) { EXPECT(order++ == 0); }
TEST(test_order1) { EXPECT(order++ == 1); }

RUN_ALL();
