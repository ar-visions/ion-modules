export module ex;

import io;
import var;

export {

	/// basic enums type needs to be enumerable by enum keyword, serializable, and introspectable
	struct EnumData:io {
		Type        type;
		int         kind;
		EnumData(Type type, int kind): type(type), kind(kind) { }
	};

	template <typename T>
	struct ex:EnumData {
		ex(int kind) : EnumData(Id<T>(), kind) { }
		operator bool() { return kind > 0; }
		operator  var() {
			for (auto &sym: T::symbols)
				if (sym.value == kind)
					return sym.symbol;
			assert(false);
			return null;
		}

		str symbol() {
			for (auto &sym: T::symbols)
				if (sym.value == kind)
					return sym.symbol;
			return null;
		}

		/// depending how Symbol * param, this is an assertion failure on a resolve of symbol to int
		int resolve(std::string s, Symbol **ptr = null) {
			for (auto &sym: T::symbols)
				if (sym.symbol == s) {
					if (ptr)
						*ptr = &sym;
					return sym.value;
				}
			if (ptr)
			   *ptr = null;
			///
			assert(!ptr || T::symbols.size());
			return T::symbols[0].value;
		}
		///
		ex(var &v) { kind = resolve(v); }
	};
}
