#include "loader/symbolDatabase.h"

#include "common/file.h"

#include <fmt/format.h>
#include <magic_enum.hpp>

namespace Loader {

constexpr char LIB_PREFIX[] = "libSce";

static std::string UpdateName(const std::string& str) {
	return str.starts_with(LIB_PREFIX) ? Common::RemoveFirst(str, 6) : str;
}

std::string SymbolDatabase::GenerateName(const SymbolResolve& s) {
	auto library = UpdateName(s.library);
	auto module  = UpdateName(s.module);
	return fmt::format("{}[{}_v{}][{}_v{}.{}][{}]", s.name.c_str(), library.c_str(),
	                   s.library_version, module.c_str(), s.module_version_major,
	                   s.module_version_minor, magic_enum::enum_name(s.type));
}

// The generated name is "token[...]"; the leading token is what FindByNid/FindByName match on.
static std::string LeadingToken(const std::string& generated_name) {
	const auto bracket = generated_name.find('[');
	return generated_name.substr(0, bracket == std::string::npos ? generated_name.size() : bracket);
}

void SymbolDatabase::Register(SymbolRecord r) {
	const auto index = m_symbols.size();
	const auto token = LeadingToken(r.name);
	m_map.insert_or_assign(r.name, index);
	m_nid_index[token].push_back(index);
	m_name_index[token].push_back(index);
	m_symbols.push_back(std::move(r));
}

void SymbolDatabase::Add(const SymbolResolve& s, uint64_t vaddr) {
	SymbolRecord r {};
	r.name  = GenerateName(s);
	r.vaddr = vaddr;
	Register(std::move(r));
}

void SymbolDatabase::Add(const SymbolResolve& s, uint64_t vaddr, const std::string& dbg_name) {
	SymbolRecord r {};
	r.name     = GenerateName(s);
	r.vaddr    = vaddr;
	r.dbg_name = dbg_name;
	Register(std::move(r));
}

void SymbolDatabase::DbgDump(const std::string& folder, const std::string& file_name) {
	auto folder_str = Common::FixDirectorySlash(folder);

	Common::File::CreateDirectories(folder_str);

	Common::File f;
	f.Create(folder_str + file_name);

	for (const auto& sym: m_symbols) {
		f.Printf("%" PRIx64 " %s\n", sym.vaddr, sym.name.c_str());
	}

	f.Close();
}

const SymbolRecord* SymbolDatabase::Find(const SymbolResolve& s) const {
	auto it = m_map.find(GenerateName(s));
	if (it == m_map.end()) {
		return nullptr;
	}
	auto index = it->second;
	if (index >= m_symbols.size()) {
		return nullptr;
	}
	return &m_symbols[index];
}

const SymbolRecord* SymbolDatabase::FindByNid(const std::string& nid, SymbolType type,
                                                uint64_t vaddr) const {
	const auto it = m_nid_index.find(nid);
	if (it == m_nid_index.end()) {
		return nullptr;
	}

	auto prefix = nid + "[";
	auto suffix = fmt::format("[{}]", magic_enum::enum_name(type));

	for (const auto index: it->second) {
		const auto& symbol = m_symbols[index];
		if ((vaddr == 0 || symbol.vaddr == vaddr) && symbol.name.starts_with(prefix) &&
		    symbol.name.ends_with(suffix)) {
			return &symbol;
		}
	}

	return nullptr;
}

const SymbolRecord* SymbolDatabase::FindByName(const std::string& name, SymbolType type) const {
	const auto it = m_name_index.find(name);
	if (it == m_name_index.end()) {
		return nullptr;
	}

	auto prefix = name + "[";
	auto suffix = fmt::format("[{}]", magic_enum::enum_name(type));

	for (const auto index: it->second) {
		const auto& symbol = m_symbols[index];
		if (symbol.name.starts_with(prefix) && symbol.name.ends_with(suffix)) {
			return &symbol;
		}
	}

	return nullptr;
}

} // namespace Loader
