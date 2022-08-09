#pragma warning(disable:5050)

import dx;
import web;

///
struct EnumType {
	/// this Value is required as part of this structured enum.
	enum Value {
		Abc,
		Abc123
	};

};

///
struct Delta :mx {
	///
	struct M :Members {
		list<int> data;
		idx		  fifo;
		
		///
		Meta meta() {
			return {
				{ "data", data, list<int>()  },
				{ "fifo", fifo,       idx(2) }
			};
		}
		
		/// access control based on enumerable..
		M() : Members(Extern) { }
	} &m;

	///
	Delta(initial<Field> f) : m(set<Delta, M>(f)) { }
};

int main(int argc, cchar_t *argv[]) {
	/// member collections used for different purposes.
	Delta d = {
		{ "data", list<int> { 1, 2, 3 }},
		{ "fifo", 1 }
	};

	idx fifo = d.m.fifo;

	console.print(d.m.fifo);
	///
	///console.test(1, "wtf");
    return true;
}
